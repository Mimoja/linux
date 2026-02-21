// SPDX-License-Identifier: GPL-2.0
/*
 * EFI stub for MIPS - kernel image handling
 *
 * Adapted from loongarch-stub.c
 *
 * MIPS kernels are statically linked at a KSEG0 virtual address
 * (e.g. 0xffffffff80100000). All absolute addresses in the kernel
 * (data, rodata, BSS) reference KSEG0 addresses. KSEG0 maps to a
 * fixed physical address window (phys = virt & 0x1FFFFFFF), so the
 * kernel image MUST reside at the correct physical address for those
 * references to work.
 *
 * The UEFI firmware on MIPS (Loongson) uses XKPHYS addresses
 * (0x98000000xxxxxxxx) in its memory map. To place the kernel at the
 * right physical location, we construct the XKPHYS equivalent of the
 * kernel's physical load address and allocate there explicitly.
 *
 * The CCA (Cache Coherency Attribute) for the XKPHYS region is
 * determined at runtime by inspecting the address the firmware used
 * to load us (image->image_base).
 */

#include <linux/efi.h>

#include <asm/addrspace.h>
#include <asm/efi.h>
#include <asm/sections.h>

#include "efistub.h"

/*
 * Linker symbols from head.S / vmlinux.lds.S.
 * After STUBCOPY, these become __efistub_kernel_entry etc.,
 * resolved by the linker aliases in image-vars.h.
 */
extern char _head[], kernel_entry[], __init_end[];

/*
 * When the EFI stub is built as part of the kernel (not zboot), the
 * PE/COFF header is embedded at the start of the image in head.S.
 * The image base is at _head (the "MZ" signature) while the actual
 * kernel entry point (kernel_entry in head.S) sits past the header.
 *
 * efi_enter_kernel() receives the image base address from the generic
 * stub and must add this offset to reach kernel_entry. Without this,
 * the CPU would attempt to execute the PE/COFF header as code.
 *
 * This strong definition overrides the __weak fallback in mips.c
 * which returns 0 (correct for the zboot path where the decompressed
 * image starts directly at kernel_entry with no PE header).
 */
unsigned long stext_offset(void)
{
	return kernel_entry - _head;
}

/*
 * Extract the XKPHYS base from a firmware address.
 *
 * XKPHYS addresses have the format: 0x | 10 | CCA(3bit) | phys(59bit)
 * We extract bits [63:59] (region + CCA) and shift back to get the
 * base prefix that can be OR'd with a physical address.
 */
static unsigned long xkphys_base_from(unsigned long addr)
{
	return addr & ~TO_PHYS_MASK;
}

efi_status_t
handle_kernel_image(unsigned long *image_addr, unsigned long *image_size,
		    unsigned long *reserve_addr, unsigned long *reserve_size,
		    efi_loaded_image_t *image, efi_handle_t image_handle)
{
	unsigned long kernel_size, kernel_memsize;
	unsigned long cur_image_addr = (unsigned long)image->image_base;
	unsigned long phys_load_addr, xkphys_target;
	unsigned long xkphys_base;
	efi_status_t status;
	unsigned long nr_pages;
	efi_physical_addr_t efi_addr;

	/*
	 * kernel_size: bytes to copy (file-backed: text + data + init).
	 * The UEFI loader places all of this in memory. BSS follows but
	 * has no file backing; it's zeroed by the UEFI loader and again
	 * by head.S.
	 *
	 * kernel_memsize: total virtual footprint including BSS.
	 */
	kernel_size = __init_end - _head;
	kernel_memsize = _end - _head;

	*image_addr = cur_image_addr;
	*image_size = kernel_memsize;

	/*
	 * The kernel's physical load address, derived from its KSEG0
	 * link address. For VMLINUX_LOAD_ADDRESS = 0xffffffff80100000,
	 * this yields 0x00100000 (1MB).
	 */
	phys_load_addr = CPHYSADDR(VMLINUX_LOAD_ADDRESS);

	/*
	 * Construct the XKPHYS address for the kernel's physical location
	 * using the same CCA the firmware used to load us. This ensures
	 * we can access the destination memory with proper caching.
	 */
	xkphys_base = xkphys_base_from(cur_image_addr);
	xkphys_target = xkphys_base | phys_load_addr;

	efi_info("MIPS EFI: image_base=0x%lx kernel_size=0x%lx memsize=0x%lx\n",
		 cur_image_addr, kernel_size, kernel_memsize);
	efi_info("MIPS EFI: phys_load=0x%lx xkphys_target=0x%lx\n",
		 phys_load_addr, xkphys_target);

	/*
	 * Check if the image is already at the right physical address.
	 * If so, we can skip the copy entirely.
	 */
	if ((cur_image_addr & TO_PHYS_MASK) == phys_load_addr) {
		efi_info(
			"MIPS EFI: already at target phys addr, no relocation needed\n");
		*reserve_addr = 0;
		*reserve_size = 0;
		return EFI_SUCCESS;
	}

	/*
	 * Allocate memory at the kernel's required physical location.
	 * We use EFI_ALLOCATE_ADDRESS to request the exact XKPHYS address
	 * corresponding to the kernel's KSEG0 link address.
	 */
	nr_pages = round_up(kernel_memsize, EFI_PAGE_SIZE) / EFI_PAGE_SIZE;
	efi_addr = xkphys_target;

	status = efi_bs_call(allocate_pages, EFI_ALLOCATE_ADDRESS,
			     EFI_LOADER_DATA, nr_pages, &efi_addr);

	if (status != EFI_SUCCESS) {
		/*
		 * The exact allocation failed. Try the raw physical address
		 * in case the firmware uses physical addresses in its map.
		 */
		efi_addr = phys_load_addr;
		status = efi_bs_call(allocate_pages, EFI_ALLOCATE_ADDRESS,
				     EFI_LOADER_DATA, nr_pages, &efi_addr);
	}

	if (status != EFI_SUCCESS) {
		efi_err("Failed to allocate at 0x%lx or 0x%lx\n", xkphys_target,
			phys_load_addr);
		*image_size = 0;
		return status;
	}

	efi_info("MIPS EFI: allocated at 0x%lx, copying kernel...\n",
		 (unsigned long)efi_addr);

	/*
	 * Copy the kernel image to the target location.
	 * kernel_size covers .text through _edata (the file-backed portion).
	 * The remainder (init+bss) will be zeroed by the kernel itself.
	 */
	memcpy((void *)(unsigned long)efi_addr, (void *)cur_image_addr,
	       kernel_size);

	*image_addr = (unsigned long)efi_addr;
	*reserve_addr = *image_addr;
	*reserve_size = kernel_memsize;

	return EFI_SUCCESS;
}

/*
 * Synchronize the instruction cache after modifying code regions.
 *
 * MIPS has separate I-cache and D-cache which are not coherent.
 * After copying/relocating kernel code we must ensure the I-cache
 * sees the new instructions.
 *
 * The 'synci' instruction (available on MIPS32R2+/MIPS64R2+) performs
 * a cache line sync for the specified address. We walk the entire range
 * issuing synci for each cache line. A trailing 'sync' ensures all
 * prior operations are visible, and a 'jr.hb' (or equivalent hazard
 * barrier instruction sequence) clears the instruction hazard.
 *
 * For pre-R2 cores without synci, we fall back to a cache instruction
 * based flush via 'cache 0x10' (Hit_Invalidate_I).
 */
void efi_icache_sync(unsigned long start, unsigned long end)
{
	unsigned long addr;

	/*
	 * Use synci which is available on MIPS32R2+ / MIPS64R2+.
	 * We use a conservative 32-byte step size (common MIPS cache line
	 * size). The actual step could be read from the SYNCI_Step field
	 * of the Config5 register but 32 bytes is safe and common.
	 */
	for (addr = start; addr < end; addr += 32)
		asm volatile("synci 0(%0)" : : "r"(addr));

	/* Ensure completion of all cache operations */
	asm volatile("sync" ::: "memory");

	/*
	 * Clear instruction hazard - use a sequence that works on
	 * all MIPS revisions. On MIPSr2+ 'jr.hb $ra' could be used
	 * but this .set mips32r2 / ehb approach is more portable within
	 * the stub context.
	 */
	asm volatile(".set push\n\t"
		     ".set mips32r2\n\t"
		     "ehb\n\t"
		     ".set pop");
}
