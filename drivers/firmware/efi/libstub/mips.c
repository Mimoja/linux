// SPDX-License-Identifier: GPL-2.0
/*
 * EFI stub for MIPS - platform features and kernel entry
 *
 * Adapted from riscv.c and loongarch.c
 */

#include <linux/efi.h>
#include <linux/libfdt.h>

#include <asm/efi.h>

#include "efistub.h"

typedef void __noreturn (*kernel_entry_t)(long fw_arg0, unsigned long fw_arg1,
					  unsigned long fw_arg2,
					  unsigned long fw_arg3);

/*
 * When the kernel is booted via EFI zboot, the compressed payload is
 * extracted into memory and execution starts at offset 0 of the
 * decompressed image. In that case stext_offset() should return 0.
 *
 * When the kernel is booted directly as a PE/COFF image (with the PE
 * header embedded in head.S), the real kernel_entry is located past
 * the PE/COFF header. The strong override in mips-stub.c provides
 * the actual offset for that case.
 */
unsigned long __weak stext_offset(void)
{
	return 0;
}

efi_status_t check_platform_features(void)
{
	/* No special platform checks needed for generic MIPS */
	return EFI_SUCCESS;
}

/*
 * Enter the MIPS kernel with FDT pointer.
 *
 * Arguments passed to the kernel (saved in fw_arg0-fw_arg3 by head.S):
 *   $a0 = -2         (UHI magic, indicates FDT boot)
 *   $a1 = FDT address (used by generic MIPS get_fdt() via UHI convention)
 *   $a2 = FDT address (used by Loongson prom_dtb_init_env() which reads fw_arg2)
 *   $a3 = 0
 *
 * Passing the FDT in both $a1 and $a2 satisfies both boot protocols:
 * - Generic MIPS: get_fdt() checks fw_arg0 == -2, uses fw_arg1
 * - Loongson64: prom_dtb_init_env() checks if fw_arg2 is a valid
 *   KSEG0/XKPHYS address and uses it directly as the DTB pointer
 */
void __noreturn efi_enter_kernel(unsigned long entrypoint,
				 unsigned long fdt_addr, unsigned long fdt_size)
{
	unsigned long kentry = entrypoint + stext_offset();
	kernel_entry_t kernel = (kernel_entry_t)kentry;

	kernel(-2, fdt_addr, fdt_addr, 0);
}
