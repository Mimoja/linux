/* SPDX-License-Identifier: GPL-2.0 */
/*
 * PE/COFF header for MIPS EFI stub
 *
 * Adapted from arch/loongarch/kernel/efi-header.S and
 *              arch/riscv/kernel/efi-header.S
 *
 * The __EFI_PE_HEADER macro generates a PE/COFF header that allows
 * UEFI firmware to load the kernel image directly via LoadImage().
 * It is invoked from head.S immediately after the DOS header fields.
 *
 * The PE image has two sections:
 *   .text  - covers kernel text, rodata, and data (up to __pecoff_text_end)
 *   .data  - covers init sections and BSS (from __pecoff_text_end to _end)
 *
 * MIPS has a section layout where .init comes after .data, so the PE
 * .text section encompasses both code and data. This is functionally
 * correct — UEFI only needs to know what to load and where.
 */

#include <linux/pe.h>
#include <linux/sizes.h>

	.macro	__EFI_PE_HEADER
	.long	IMAGE_NT_SIGNATURE		/* "PE\0\0" */
.Lcoff_header:
#ifdef CONFIG_64BIT
	.short	IMAGE_FILE_MACHINE_R4000	/* MIPS little-endian 64-bit */
#else
	.short	IMAGE_FILE_MACHINE_MIPSFPU	/* MIPS little-endian 32-bit */
#endif
	.short	.Lsection_count			/* NumberOfSections */
	.long	0				/* TimeDateStamp */
	.long	0				/* PointerToSymbolTable */
	.long	0				/* NumberOfSymbols */
	.short	.Lsection_table - .Loptional_header /* SizeOfOptionalHeader */
	.short	IMAGE_FILE_DEBUG_STRIPPED | \
		IMAGE_FILE_EXECUTABLE_IMAGE | \
		IMAGE_FILE_LINE_NUMS_STRIPPED	/* Characteristics */

.Loptional_header:
	/*
	 * PE32+ (64-bit) or PE32 (32-bit) optional header.
	 * The magic value tells the loader which pointer size to expect
	 * in the remaining header fields.
	 */
#ifdef CONFIG_64BIT
	.short	IMAGE_NT_OPTIONAL_HDR64_MAGIC	/* PE32+ format */
#else
	.short	IMAGE_NT_OPTIONAL_HDR32_MAGIC	/* PE32 format */
#endif
	.byte	0x02				/* MajorLinkerVersion */
	.byte	0x14				/* MinorLinkerVersion */
	.long	__pecoff_text_end - .Lefi_header_end /* SizeOfCode */
	.long	__pecoff_data_virt_size		/* SizeOfInitializedData */
	.long	0				/* SizeOfUninitializedData */
	/*
	 * AddressOfEntryPoint: offset from image base to the EFI stub
	 * entry trampoline efi_entry() in libstub. The trampoline fixes
	 * up the GOT for PIC code before calling the C efi_pe_entry().
	 * The __efistub_ prefix is applied by objcopy during the stub build.
	 */
	.long	__efistub_efi_entry - _head	/* AddressOfEntryPoint */
	.long	.Lefi_header_end - _head	/* BaseOfCode */
#ifdef CONFIG_32BIT
	.long	__pecoff_text_end - _head	/* BaseOfData (PE32 only) */
#endif

.Lextra_header_fields:
	.quad	0				/* ImageBase */
	.long	PECOFF_SEGMENT_ALIGN		/* SectionAlignment */
	.long	PECOFF_FILE_ALIGN		/* FileAlignment */
	.short	0				/* MajorOperatingSystemVersion */
	.short	0				/* MinorOperatingSystemVersion */
	.short	LINUX_EFISTUB_MAJOR_VERSION	/* MajorImageVersion */
	.short	LINUX_EFISTUB_MINOR_VERSION	/* MinorImageVersion */
	.short	0				/* MajorSubsystemVersion */
	.short	0				/* MinorSubsystemVersion */
	.long	0				/* Win32VersionValue */

	.long	_end - _head			/* SizeOfImage */

	/*
	 * SizeOfHeaders: everything before the first section's raw data.
	 * The PE header + section table is padded up to efi_header_end
	 * which is 64K-aligned, so all of that counts as "headers".
	 */
	.long	.Lefi_header_end - _head	/* SizeOfHeaders */
	.long	0				/* CheckSum */
	.short	IMAGE_SUBSYSTEM_EFI_APPLICATION	/* Subsystem */
	.short	IMAGE_DLLCHARACTERISTICS_NX_COMPAT /* DllCharacteristics */
	.quad	0				/* SizeOfStackReserve */
	.quad	0				/* SizeOfStackCommit */
	.quad	0				/* SizeOfHeapReserve */
	.quad	0				/* SizeOfHeapCommit */
	.long	0				/* LoaderFlags */
	.long	(.Lsection_table - .) / 8	/* NumberOfRvaAndSizes */

	.quad	0				/* ExportTable */
	.quad	0				/* ImportTable */
	.quad	0				/* ResourceTable */
	.quad	0				/* ExceptionTable */
	.quad	0				/* CertificationTable */
	.quad	0				/* BaseRelocationTable */

	/* Section table */
.Lsection_table:
	/*
	 * PE .text section: covers everything from the end of the PE
	 * headers to __pecoff_text_end. On MIPS this includes the kernel
	 * .text, .rodata, and .data sections because the MIPS linker
	 * script places .init after .data (unlike ARM64/RISC-V/LoongArch
	 * which place .init before .data).
	 */
	.ascii	".text\0\0\0"
	.long	__pecoff_text_end - .Lefi_header_end /* VirtualSize */
	.long	.Lefi_header_end - _head	/* VirtualAddress */
	.long	__pecoff_text_end - .Lefi_header_end /* SizeOfRawData */
	.long	.Lefi_header_end - _head	/* PointerToRawData */
	.long	0				/* PointerToRelocations */
	.long	0				/* PointerToLineNumbers */
	.short	0				/* NumberOfRelocations */
	.short	0				/* NumberOfLineNumbers */
	.long	IMAGE_SCN_CNT_CODE | \
		IMAGE_SCN_MEM_READ | \
		IMAGE_SCN_MEM_EXECUTE		/* Characteristics */

	/*
	 * PE .data section: covers init sections and BSS.
	 * SizeOfRawData is the file-backed portion (init text + init data).
	 * VirtualSize extends through BSS which is zero-initialized by the
	 * loader but occupies no space in the file.
	 */
	.ascii	".data\0\0\0"
	.long	__pecoff_data_virt_size		/* VirtualSize */
	.long	__pecoff_text_end - _head	/* VirtualAddress */
	.long	__pecoff_data_raw_size		/* SizeOfRawData */
	.long	__pecoff_text_end - _head	/* PointerToRawData */
	.long	0				/* PointerToRelocations */
	.long	0				/* PointerToLineNumbers */
	.short	0				/* NumberOfRelocations */
	.short	0				/* NumberOfLineNumbers */
	.long	IMAGE_SCN_CNT_INITIALIZED_DATA | \
		IMAGE_SCN_MEM_READ | \
		IMAGE_SCN_MEM_WRITE		/* Characteristics */

	.set	.Lsection_count, (. - .Lsection_table) / 40

	/*
	 * Align to 64K (PECOFF_SEGMENT_ALIGN). MIPS supports page sizes
	 * up to 64K, so section alignment must be at least this large.
	 * This matches LoongArch which also uses 0x10000.
	 */
	.balign	0x10000
.Lefi_header_end:
	.endm
