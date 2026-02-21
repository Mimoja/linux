/* SPDX-License-Identifier: GPL-2.0 */
#include <asm/asm-offsets.h>
#include <asm/thread_info.h>

#define PAGE_SIZE _PAGE_SIZE

/*
 * Put .bss..swapper_pg_dir as the first thing in .bss. This will
 * ensure that it has .bss alignment (64K).
 */
#define BSS_FIRST_SECTIONS *(.bss..swapper_pg_dir)

/* Cavium Octeon should not have a separate PT_NOTE Program Header. */
#ifndef CONFIG_CAVIUM_OCTEON_SOC
#define EMITS_PT_NOTE
#endif

#define RUNTIME_DISCARD_EXIT

#include <asm-generic/vmlinux.lds.h>

#ifdef CONFIG_EFI_STUB
#include "image-vars.h"

/*
 * PE/COFF section and file alignment constants. MIPS supports page
 * sizes up to 64K, so section alignment must be at least that large
 * to guarantee each PE section starts on a page boundary.
 * File alignment of 512 bytes is the PE/COFF standard minimum.
 */
PECOFF_FILE_ALIGN = 0x200;
PECOFF_SEGMENT_ALIGN = 0x10000;
#endif

#undef mips
#define mips mips
OUTPUT_ARCH(mips)
ENTRY(kernel_entry)
PHDRS {
	text PT_LOAD FLAGS(7);	/* RWX */
#ifndef CONFIG_CAVIUM_OCTEON_SOC
	note PT_NOTE FLAGS(4);	/* R__ */
#endif /* CAVIUM_OCTEON_SOC */
}

#ifdef CONFIG_32BIT
	#ifdef CONFIG_CPU_LITTLE_ENDIAN
		jiffies	 = jiffies_64;
	#else
		jiffies	 = jiffies_64 + 4;
	#endif
#else
	jiffies	 = jiffies_64;
#endif

SECTIONS
{
#ifdef CONFIG_BOOT_ELF64
	/* Read-only sections, merged into text segment: */
	/* . = 0xc000000000000000; */

	/* This is the value for an Origin kernel, taken from an IRIX kernel.  */
	/* . = 0xc00000000001c000; */

	/* Set the vaddr for the text segment to a value
	 *   >= 0xa800 0000 0001 9000 if no symmon is going to configured
	 *   >= 0xa800 0000 0030 0000 otherwise
	 */

	/* . = 0xa800000000300000; */
	. = 0xffffffff80300000;
#endif
	. = LINKER_LOAD_ADDRESS;
	/* read-only */
	_text = .;	/* Text and read-only data */

#ifdef CONFIG_EFI_STUB
	/*
	 * When EFI stub is enabled, the PE/COFF header lives in .head.text
	 * and must be at the very start of the image (offset 0 = _text).
	 * Place it in its own output section before .text so that _head
	 * (the "MZ" signature in head.S) sits at the image base address.
	 */
	HEAD_TEXT_SECTION
#endif

	.text : {
#ifndef CONFIG_EFI_STUB
		HEAD_TEXT
#endif
		TEXT_TEXT
		SCHED_TEXT
		LOCK_TEXT
		KPROBES_TEXT
		IRQENTRY_TEXT
		SOFTIRQENTRY_TEXT
		*(.fixup)
		*(.gnu.warning)
#ifndef CONFIG_EFI_STUB
		. = ALIGN(16);
		*(.got)	/* Global offset table */
#endif
	} :text = 0
	_etext = .;	/* End of text section */

	EXCEPTION_TABLE(16)

	/* Exception table for data bus errors */
	__dbe_table : {
		__start___dbe_table = .;
		KEEP(*(__dbe_table))
		__stop___dbe_table = .;
	}

	_sdata = .;			/* Start of data section */
	RO_DATA(4096)

	/* writeable */
	.data : {	/* Data */
		. = . + DATAOFFSET;		/* for CONFIG_MAPPED_KERNEL */

		INIT_TASK_DATA(THREAD_SIZE)
		NOSAVE_DATA
		PAGE_ALIGNED_DATA(PAGE_SIZE)
		CACHELINE_ALIGNED_DATA(1 << CONFIG_MIPS_L1_CACHE_SHIFT)
		READ_MOSTLY_DATA(1 << CONFIG_MIPS_L1_CACHE_SHIFT)
		DATA_DATA
		CONSTRUCTORS
	}
	BUG_TABLE
#ifndef CONFIG_EFI_STUB
	_gp = . + 0x8000;
#endif
	.lit8 : {
		*(.lit8)
	}
	.lit4 : {
		*(.lit4)
	}
	/* We want the small data sections together, so single-instruction offsets
	   can access them all, and initialized data all before uninitialized, so
	   we can shorten the on-disk segment size.  */
	.sdata : {
		*(.sdata)
	}
	_edata =  .;			/* End of data section */

#ifdef CONFIG_EFI_STUB
	/*
	 * PE/COFF .text section boundary. Everything from efi_header_end
	 * to here is covered by the PE .text section. Must be aligned to
	 * PECOFF_SEGMENT_ALIGN so the PE .data section that follows starts
	 * on a section-aligned boundary.
	 */
	. = ALIGN(PECOFF_SEGMENT_ALIGN);
	__pecoff_text_end = .;
#endif

	/* will be freed after init */
	. = ALIGN(PAGE_SIZE);		/* Init code and data */
	__init_begin = .;
	INIT_TEXT_SECTION(PAGE_SIZE)

#ifdef CONFIG_EFI_STUB
	/*
	 * With CONFIG_EFI_STUB, the EFI stub is compiled with
	 * -fpic -mabicalls and uses GOT-relative addressing. The GOT
	 * and _gp must be within ±32KB of both the stub code (in
	 * .init.text) and the GOT entries. Place them here, right
	 * after .init.text and before .init.data.
	 *
	 * The kernel proper uses -mno-abicalls so it does not reference
	 * _gp or .got at all; moving them here is safe.
	 *
	 * After boot, the efi_entry trampoline fixes up all GOT entries
	 * by adding the load-time delta (actual address - link address).
	 */
	. = ALIGN(16);
	.got : AT(ADDR(.got) - LOAD_OFFSET) {
		_got_start = .;
		*(.got)
		_got_end = .;
	}
	_gp = _got_start + 0x7ff0;
#endif

	INIT_DATA_SECTION(16)

	. = ALIGN(4);
	.mips.machines.init : AT(ADDR(.mips.machines.init) - LOAD_OFFSET) {
		__mips_machines_start = .;
		KEEP(*(.mips.machines.init))
		__mips_machines_end = .;
	}

	/* .exit.text is discarded at runtime, not link time, to deal with
	 * references from .rodata
	 */
	.exit.text : {
		EXIT_TEXT
	}
	.exit.data : {
		EXIT_DATA
	}
#ifdef CONFIG_SMP
	PERCPU_SECTION(1 << CONFIG_MIPS_L1_CACHE_SHIFT)
#endif

	.rel.dyn : ALIGN(8) {
		*(.rel)
		*(.rel*)
	}

#ifdef CONFIG_MIPS_ELF_APPENDED_DTB
	STRUCT_ALIGN();
	.appended_dtb : AT(ADDR(.appended_dtb) - LOAD_OFFSET) {
		*(.appended_dtb)
		KEEP(*(.appended_dtb))
	}
#endif

#ifdef CONFIG_RELOCATABLE
	. = ALIGN(4);

	.data.reloc : {
		_relocation_start = .;
		/*
		 * Space for relocation table
		 * This needs to be filled so that the
		 * relocs tool can overwrite the content.
		 * An invalid value is left at the start of the
		 * section to abort relocation if the table
		 * has not been filled in.
		 */
		LONG(0xFFFFFFFF);
		FILL(0);
		. += CONFIG_RELOCATION_TABLE_SIZE - 4;
		_relocation_end = .;
	}
#endif

#ifdef CONFIG_MIPS_RAW_APPENDED_DTB
	.fill : {
		FILL(0);
		BYTE(0);
		STRUCT_ALIGN();
	}
	__appended_dtb = .;
	/* leave space for appended DTB */
	. += 0x100000;
#endif
	/*
	 * Align to 64K in attempt to eliminate holes before the
	 * .bss..swapper_pg_dir section at the start of .bss.  This
	 * also satisfies PAGE_SIZE alignment as the largest page size
	 * allowed is 64K.
	 */
	. = ALIGN(0x10000);
	__init_end = .;
	/* freed after init ends here */

#ifdef CONFIG_EFI_STUB
	/*
	 * PE/COFF .data section raw (file-backed) size. This padding
	 * ensures the raw data ends on a PE file alignment boundary.
	 * Everything from __pecoff_text_end to here has file backing
	 * (init text, init data, etc.) and is loaded by the UEFI loader.
	 */
	.pecoff_edata_padding : { BYTE(0); . = ALIGN(PECOFF_FILE_ALIGN); }
	__pecoff_data_raw_size = ABSOLUTE(. - __pecoff_text_end);
#endif

	/*
	 * Force .bss to 64K alignment so that .bss..swapper_pg_dir
	 * gets that alignment.	 .sbss should be empty, so there will be
	 * no holes after __init_end. */
	BSS_SECTION(0, 0x10000, 8)

#ifdef CONFIG_EFI_STUB
	/*
	 * PE/COFF .data section virtual size. This extends through BSS
	 * which is zero-initialized by the UEFI loader but has no file
	 * backing. The difference between virt_size and raw_size tells
	 * the loader how much extra memory to allocate and zero-fill.
	 */
	. = ALIGN(PECOFF_SEGMENT_ALIGN);
	__pecoff_data_virt_size = ABSOLUTE(. - __pecoff_text_end);
#endif

	_end = . ;

#ifdef CONFIG_EFI_STUB
	/*
	 * Absolute values for head.S PE header fields. GAS does not
	 * allow cross-section differences in data directives, so we
	 * compute these in the linker script where all section
	 * addresses are resolved.
	 */
	_kernel_entry_offset = ABSOLUTE(kernel_entry - _head);
	_kernel_image_size = ABSOLUTE(_end - _head);
#endif

	/* These mark the ABI of the kernel for debuggers.  */
	.mdebug.abi32 : {
		KEEP(*(.mdebug.abi32))
	}
	.mdebug.abi64 : {
		KEEP(*(.mdebug.abi64))
	}

	/* This is the MIPS specific mdebug section.  */
	.mdebug : {
		*(.mdebug)
	}

	STABS_DEBUG
	DWARF_DEBUG
	ELF_DETAILS

	/* These must appear regardless of  .  */
	.gptab.sdata : {
		*(.gptab.data)
		*(.gptab.sdata)
	}
	.gptab.sbss : {
		*(.gptab.bss)
		*(.gptab.sbss)
	}

	/* Sections to be discarded */
	DISCARDS
	/DISCARD/ : {
		/* ABI crap starts here */
		*(.MIPS.abiflags)
		*(.MIPS.options)
		*(.gnu.attributes)
		*(.options)
		*(.pdr)
		*(.reginfo)
	}
}
