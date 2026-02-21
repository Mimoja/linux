/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025 MIPS EFI stub support
 * Adapted from arch/riscv/include/asm/efi.h
 */
#ifndef _ASM_MIPS_EFI_H
#define _ASM_MIPS_EFI_H

#include <asm/addrspace.h>
#include <asm/mipsregs.h>

/*
 * MIPS-specific EFI memory range conversion.
 *
 * The UEFI firmware on MIPS (Loongson) uses XKPHYS virtual addresses
 * (e.g. 0x980000000XXXXXXX) in its memory map rather than raw physical
 * addresses. The kernel's memblock subsystem expects physical addresses.
 * Strip the XKPHYS prefix bits to convert to physical before the
 * generic code adds these ranges to memblock.
 *
 * XKPHYS addresses have bits [63:62] = 0b10 and bits [61:59] encode the
 * CCA. TO_PHYS_MASK (0x07ffffffffffffff) strips all of these.
 * If the address is already physical (bit 63 = 0), the mask is a no-op.
 */
#ifdef CONFIG_64BIT
#define memrange_efi_to_native(addr, npages)                                  \
	do {                                                                  \
		*(npages) = PFN_UP(*(addr) + (*(npages) << EFI_PAGE_SHIFT)) - \
			    PFN_DOWN(*(addr));                                \
		*(addr) &= TO_PHYS_MASK;                                      \
		*(addr) &= PAGE_MASK;                                         \
	} while (0)
#endif

#include <linux/efi.h>

#ifdef CONFIG_EFI
extern void efi_init(void);
#else
#define efi_init()
#endif

int efi_create_mapping(struct mm_struct *mm, efi_memory_desc_t *md);
int efi_set_mapping_permissions(struct mm_struct *mm, efi_memory_desc_t *md,
				bool);

/*
 * MIPS CP0 Status register IE bit controls global interrupt enable.
 * This mask is used to save/restore interrupt state around EFI runtime
 * service calls.
 */
#define ARCH_EFI_IRQ_FLAGS_MASK ST0_IE

/* Load initrd anywhere in system RAM */
static inline unsigned long efi_get_max_initrd_addr(unsigned long image_addr)
{
	return ULONG_MAX;
}

static inline unsigned long efi_get_kimg_min_align(void)
{
	/*
	 * MIPS requires the kernel image to be placed at a 64K aligned
	 * address. This is sufficient for both MIPS32 and MIPS64.
	 */
	return SZ_64K;
}

/*
 * The kernel is linked at a KSEG0 virtual address (e.g. 0xffffffff80100000).
 * CPHYSADDR strips the segment bits to yield the physical load address
 * (e.g. 0x00100000). This is used as the preferred physical address hint
 * for efi_relocate_kernel(). On MIPS, the preferred allocation will
 * typically fail because the UEFI firmware memory map uses XKPHYS
 * addresses, so handle_kernel_image() does its own targeted allocation.
 */
#define EFI_KIMG_PREFERRED_ADDRESS CPHYSADDR(VMLINUX_LOAD_ADDRESS)

/*
 * MIPS does not use a fixed virtual offset for EFI runtime mappings.
 * The generic stub will allocate virtual addresses dynamically.
 */

void arch_efi_call_virt_setup(void);
void arch_efi_call_virt_teardown(void);

void efi_icache_sync(unsigned long start, unsigned long end);

#endif /* _ASM_MIPS_EFI_H */
