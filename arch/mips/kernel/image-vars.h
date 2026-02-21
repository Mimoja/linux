/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Linker script variables to be set after section resolution, as
 * ld.lld does not like variables assigned before SECTIONS is processed.
 *
 * Based on arch/riscv/kernel/image-vars.h
 */
#ifndef __MIPS_KERNEL_IMAGE_VARS_H
#define __MIPS_KERNEL_IMAGE_VARS_H

#ifndef LINKER_SCRIPT
#error This file should only be included in vmlinux.lds.S
#endif

#ifdef CONFIG_EFI

/*
 * The EFI stub has its own symbol namespace prefixed by __efistub_, to
 * isolate it from the kernel proper. The following symbols are legally
 * accessed by the stub, so provide some aliases to make them accessible.
 *
 * Only include data symbols here, or text symbols of functions that are
 * guaranteed to be safe when executed at another offset than they were
 * linked at. The routines below are all implemented in a position
 * independent manner.
 */

/* _head: image base where the PE/COFF "MZ" header lives (head.S) */
__efistub__head = _head;

/* kernel_entry: the real MIPS entry point past the PE header (head.S) */
__efistub_kernel_entry = kernel_entry;

/* Section boundaries used by handle_kernel_image() in mips-stub.c */
__efistub__edata = _edata;
__efistub__end = _end;
__efistub___init_end = __init_end;

/*
 * GOT boundaries for the efi_entry trampoline's self-relocation loop.
 * These are the start/end of the .init.got section which contains the
 * EFI stub's GOT entries (renamed from .got by STUBCOPY).
 */
__efistub__got_start = _got_start;
__efistub__got_end = _got_end;

#if defined(CONFIG_EFI_EARLYCON) || defined(CONFIG_SYSFB)
__efistub_screen_info = screen_info;
#endif

#endif /* CONFIG_EFI */

#endif /* __MIPS_KERNEL_IMAGE_VARS_H */
