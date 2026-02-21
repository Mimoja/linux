// SPDX-License-Identifier: GPL-2.0-only
#include <linux/init.h>
#include <linux/io.h>
#include <asm/addrspace.h>
#include <ioremap.h>

void __iomem *ioremap_prot(phys_addr_t offset, unsigned long size,
			   pgprot_t prot)
{
	unsigned long flags = pgprot_val(prot) & _CACHE_MASK;
	u64 base = (flags == _CACHE_UNCACHED ? IO_BASE : UNCAC_BASE);
	void __iomem *addr;

	addr = plat_ioremap(offset, size, flags);
	if (!addr)
		addr = (void __iomem *)(unsigned long)(base + offset);
	return addr;
}
EXPORT_SYMBOL(ioremap_prot);

void iounmap(const volatile void __iomem *addr)
{
	plat_iounmap(addr);
}
EXPORT_SYMBOL(iounmap);

#ifdef CONFIG_EFI
#include <asm-generic/early_ioremap.h>

/*
 * MIPS64 can map all of physical memory through XKPHYS, so early_ioremap
 * does not need page table setup. This follows the same approach as
 * LoongArch which uses TO_CACHE() for early mappings.
 */
void __init __iomem *early_ioremap(phys_addr_t phys_addr, unsigned long size)
{
	return ((void __iomem *)TO_CAC(phys_addr));
}

void __init early_iounmap(void __iomem *addr, unsigned long size)
{
}

void *__init early_memremap_ro(resource_size_t phys_addr, unsigned long size)
{
	return early_memremap(phys_addr, size);
}

void *__init early_memremap_prot(resource_size_t phys_addr, unsigned long size,
				 unsigned long prot_val)
{
	return early_memremap(phys_addr, size);
}
#endif
