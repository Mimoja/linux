// SPDX-License-Identifier: GPL-2.0-only
/*
 * EFI support for MIPS
 *
 * Copyright (C) 2025
 * Adapted from arch/riscv/kernel/efi.c
 */

#include <linux/efi.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/mm.h>

#include <asm/efi.h>
#include <asm/pgalloc.h>
#include <asm/pgtable.h>
#include <asm/pgtable-bits.h>
#include <asm/tlbflush.h>

/*
 * Only regions of type EFI_RUNTIME_SERVICES_CODE need to be
 * executable, everything else can be mapped without execute
 * permission. Also take the new (optional) RO/XP bits into account.
 */
static __init pgprot_t efimem_to_pgprot_map(efi_memory_desc_t *md)
{
	u64 attr = md->attribute;
	u32 type = md->type;

	if (type == EFI_MEMORY_MAPPED_IO)
		return PAGE_KERNEL;

	/*
	 * MIPS page protection is based on _PAGE_WRITE and _PAGE_NO_EXEC.
	 * If RIXI (Read-Inhibit/Execute-Inhibit) support is available,
	 * we can enforce fine-grained permissions. Otherwise fall back
	 * to PAGE_KERNEL which allows read/write/execute.
	 */

	/* R-- (read-only, no execute) */
	if ((attr & (EFI_MEMORY_XP | EFI_MEMORY_RO)) ==
	    (EFI_MEMORY_XP | EFI_MEMORY_RO))
		return __pgprot(pgprot_val(PAGE_KERNEL) & ~__WRITEABLE);

	/* R-X (read + execute, no write) */
	if (attr & EFI_MEMORY_RO)
		return __pgprot(pgprot_val(PAGE_KERNEL) & ~__WRITEABLE);

	/* RW- (read + write, no execute) */
	if (((attr & (EFI_MEMORY_RP | EFI_MEMORY_WP | EFI_MEMORY_XP)) ==
	     EFI_MEMORY_XP) ||
	    type != EFI_RUNTIME_SERVICES_CODE)
		return PAGE_KERNEL;

	/* RWX */
	return PAGE_KERNEL;
}

/*
 * Create page table mappings for EFI runtime services memory regions.
 *
 * MIPS does not provide a generic create_pgd_mapping() helper like
 * RISC-V or ARM64, so we walk the page table hierarchy and allocate
 * entries manually.
 *
 * MIPS page table peculiarities:
 *  - Empty PMD entries contain a pointer to invalid_pte_table (not zero).
 *    pgd_init()/pmd_init() set this up. We must call pmd_init() on any
 *    freshly allocated PMD page so that pmd_none() works correctly.
 *  - With PGTABLE_LEVELS=3: p4d and pud are folded into pgd. The MIPS
 *    pud_none() correctly checks pgd entries against invalid_pmd_table,
 *    even in the folded case, because pud_val() resolves through to
 *    pgd_val() of the underlying entry.
 */
static void __init mips_efi_map_page(pgd_t *pgdp, unsigned long vaddr,
				     phys_addr_t paddr, pgprot_t prot)
{
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	pgdp += pgd_index(vaddr);

	p4d = p4d_offset(pgdp, vaddr);
	if (p4d_none(*p4d)) {
		pud_t *new_pud =
			(pud_t *)__get_free_page(GFP_KERNEL | __GFP_ZERO);
		if (!new_pud)
			return;
		p4d_populate(&init_mm, p4d, new_pud);
	}

	pud = pud_offset(p4d, vaddr);
	if (pud_none(*pud)) {
		pmd_t *new_pmd =
			(pmd_t *)__get_free_page(GFP_KERNEL | __GFP_ZERO);
		if (!new_pmd)
			return;
		/*
		 * MIPS marks empty PMD entries with a pointer to
		 * invalid_pte_table rather than zero. Initialise the
		 * freshly allocated PMD page so that pmd_none() works.
		 */
		pmd_init(new_pmd);
		pud_populate(&init_mm, pud, new_pmd);
	}

	pmd = pmd_offset(pud, vaddr);
	if (pmd_none(*pmd)) {
		pte_t *new_pte =
			(pte_t *)__get_free_page(GFP_KERNEL | __GFP_ZERO);
		if (!new_pte)
			return;
		pmd_populate_kernel(&init_mm, pmd, new_pte);
	}

	pte = pte_offset_kernel(pmd, vaddr);
	set_pte(pte, pfn_pte(paddr >> PAGE_SHIFT, prot));
}

int __init efi_create_mapping(struct mm_struct *mm, efi_memory_desc_t *md)
{
	pgprot_t prot = __pgprot(pgprot_val(efimem_to_pgprot_map(md)) &
				 ~(_PAGE_GLOBAL));
	unsigned long total_size = md->num_pages << EFI_PAGE_SHIFT;
	unsigned long virt_start, phys_start, region_end;
	unsigned long nr_pages;
	phys_addr_t phys_base;
	int i;

	/*
	 * On MIPS, md->phys_addr may be an XKPHYS virtual address
	 * (e.g. 0x980000000XXXXXXX) from the UEFI firmware. Strip
	 * the XKPHYS prefix to get the raw physical address for PTEs.
	 */
	phys_base = md->phys_addr & TO_PHYS_MASK;

	/*
	 * EFI regions may start at 4KB boundaries (EFI_PAGE_SIZE), but
	 * MIPS kernel pages are 16KB.  Round virtual and physical start
	 * addresses down to kernel page boundaries, and compute the
	 * number of kernel pages needed to cover the entire region
	 * from the rounded-down start to the end.
	 */
	virt_start = md->virt_addr & PAGE_MASK;
	phys_start = phys_base & PAGE_MASK;
	region_end = md->virt_addr + total_size;
	nr_pages = DIV_ROUND_UP(region_end - virt_start, PAGE_SIZE);

	pr_info("efi_create_mapping: virt=0x%llx(->0x%lx) phys=0x%llx(->0x%lx) size=0x%lx kpages=%lu\n",
		md->virt_addr, virt_start, (u64)phys_base, phys_start,
		total_size, nr_pages);

	for (i = 0; i < nr_pages; i++) {
		unsigned long vaddr = virt_start + i * PAGE_SIZE;
		phys_addr_t paddr = phys_start + i * PAGE_SIZE;

		mips_efi_map_page(mm->pgd, vaddr, paddr, prot);
	}
	return 0;
}

int __init efi_set_mapping_permissions(struct mm_struct *mm,
				       efi_memory_desc_t *md, bool ignored)
{
	/*
	 * MIPS permission enforcement depends on RIXI support in the CPU.
	 * For now, permissions are set at mapping creation time in
	 * efi_create_mapping(). Fine-grained permission updates would
	 * require walking page tables and adjusting PTE bits.
	 *
	 * TODO: Implement proper permission adjustment when MIPS RIXI
	 * support is detected at runtime.
	 */
	return 0;
}
