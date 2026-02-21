// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2009 Lemote Inc.
 * Author: Wu Zhangjin, wuzhangjin@gmail.com
 */

#include <linux/efi.h>
#include <linux/irqchip.h>
#include <linux/libfdt.h>
#include <linux/logic_pio.h>
#include <linux/memblock.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <asm/traps.h>
#include <asm/smp-ops.h>
#include <asm/cacheflush.h>
#include <asm/fw/fw.h>

#include <loongson.h>
#include <boot_param.h>
#include <builtin_dtbs.h>
#include "smp.h"

#define NODE_ID_OFFSET_ADDR	((void __iomem *)TO_UNCAC(0x1001041c))

u32 node_id_offset;

static void __init mips_nmi_setup(void)
{
	void *base;

	base = (void *)(CAC_BASE + 0x380);
	memcpy(base, except_vec_nmi, 0x80);
	flush_icache_range((unsigned long)base, (unsigned long)base + 0x80);
}

void ls7a_early_config(void)
{
	node_id_offset = ((readl(NODE_ID_OFFSET_ADDR) >> 8) & 0x1f) + 36;
}

void rs780e_early_config(void)
{
	node_id_offset = 37;
}

void virtual_early_config(void)
{
	node_id_offset = 44;
}

void __init szmem(unsigned int node)
{
	u32 i, mem_type;
	phys_addr_t node_id, mem_start, mem_size;

	/* Otherwise come from DTB */
	if (loongson_sysconf.fw_interface != LOONGSON_LEFI)
		return;

	/* Parse memory information and activate */
	for (i = 0; i < loongson_memmap->nr_map; i++) {
		node_id = loongson_memmap->map[i].node_id;
		if (node_id != node)
			continue;

		mem_type = loongson_memmap->map[i].mem_type;
		mem_size = loongson_memmap->map[i].mem_size;

		/* Memory size comes in MB if MEM_SIZE_IS_IN_BYTES not set */
		if (mem_size & MEM_SIZE_IS_IN_BYTES)
			mem_size &= ~MEM_SIZE_IS_IN_BYTES;
		else
			mem_size = mem_size << 20;

		mem_start = (node_id << 44) | loongson_memmap->map[i].mem_start;

		switch (mem_type) {
		case SYSTEM_RAM_LOW:
		case SYSTEM_RAM_HIGH:
		case UMA_VIDEO_RAM:
			pr_info("Node %d, mem_type:%d\t[%pa], %pa bytes usable\n",
				(u32)node_id, mem_type, &mem_start, &mem_size);
			memblock_add_node(mem_start, mem_size, node,
					  MEMBLOCK_NONE);
			break;
		case SYSTEM_RAM_RESERVED:
		case VIDEO_ROM:
		case ADAPTER_ROM:
		case ACPI_TABLE:
		case SMBIOS_TABLE:
			pr_info("Node %d, mem_type:%d\t[%pa], %pa bytes reserved\n",
				(u32)node_id, mem_type, &mem_start, &mem_size);
			memblock_reserve(mem_start, mem_size);
			break;
		/* We should not reserve VUMA_VIDEO_RAM as it overlaps with MMIO */
		case VUMA_VIDEO_RAM:
		default:
			pr_info("Node %d, mem_type:%d\t[%pa], %pa bytes unhandled\n",
				(u32)node_id, mem_type, &mem_start, &mem_size);
			break;
		}
	}

	/* Reserve vgabios if it comes from firmware */
	if (loongson_sysconf.vgabios_addr)
		memblock_reserve(virt_to_phys((void *)loongson_sysconf.vgabios_addr),
				SZ_256K);
	/* set nid for reserved memory */
	memblock_set_node((u64)node << 44, (u64)(node + 1) << 44,
			&memblock.reserved, node);
}

#ifndef CONFIG_NUMA
static void __init prom_init_memory(void)
{
	szmem(0);
}
#endif

/*
 * When booting via EFI, the LEFI firmware data structures are not available.
 * Populate loongson_sysconf from CPU PRID and hardware registers so that
 * loongson3_smp_setup() can correctly set up cpu_possible_mask.
 */
static void __init prom_efi_init_sysconf(void)
{
	unsigned int prid = read_c0_prid();
	unsigned int imp = prid & PRID_IMP_MASK;

	/*
	 * Detect CPU type from PRID and set up smp_group[]/cores_per_node
	 * the same way prom_lefi_init_env() does for each cputype.
	 */
	switch (imp) {
	case PRID_IMP_LOONGSON_64R:
		/* Loongson 2K series */
		loongson_sysconf.cputype = Loongson_2K;
		smp_group[0] = 0x900000001fe11000;
		loongson_sysconf.cores_per_node = 2;
		loongson_sysconf.cores_per_package = 2;
		break;
	case PRID_IMP_LOONGSON_64C:
	case PRID_IMP_LOONGSON_64G:
		/* Loongson 3A series (classic and generic) */
		loongson_sysconf.cputype = Loongson_3A;
		loongson_sysconf.cores_per_node = 4;
		loongson_sysconf.cores_per_package = 4;
		smp_group[0] = 0x900000003ff01000;
		smp_group[1] = 0x900010003ff01000;
		smp_group[2] = 0x900020003ff01000;
		smp_group[3] = 0x900030003ff01000;
		loongson_chipcfg[0] = 0x900000001fe00180;
		loongson_chipcfg[1] = 0x900010001fe00180;
		loongson_chipcfg[2] = 0x900020001fe00180;
		loongson_chipcfg[3] = 0x900030001fe00180;
		loongson_chiptemp[0] = 0x900000001fe0019c;
		loongson_chiptemp[1] = 0x900010001fe0019c;
		loongson_chiptemp[2] = 0x900020001fe0019c;
		loongson_chiptemp[3] = 0x900030001fe0019c;
		loongson_freqctrl[0] = 0x900000001fe001d0;
		loongson_freqctrl[1] = 0x900010001fe001d0;
		loongson_freqctrl[2] = 0x900020001fe001d0;
		loongson_freqctrl[3] = 0x900030001fe001d0;
		break;
	default:
		/* Unknown CPU type — set minimum for boot CPU */
		loongson_sysconf.cputype = Loongson_3A;
		loongson_sysconf.cores_per_node = 1;
		loongson_sysconf.cores_per_package = 1;
		smp_group[0] = 0x900000003ff01000;
		loongson_chipcfg[0] = 0x900000001fe00180;
		break;
	}

	loongson_sysconf.boot_cpu_id = 0;
	loongson_sysconf.reserved_cpus_mask = 0;

	/*
	 * Discover CPU count and clock frequency from the fw_cfg device.
	 * QEMU's loongson3-virt machine provides fw_cfg at 0x10080100.
	 * The EDK2 firmware reads this same device, so it is always
	 * present and correctly populated.
	 *
	 * fw_cfg memory-mapped protocol (DEVICE_BIG_ENDIAN in QEMU):
	 *   - Write selector (big-endian u16) to base+0
	 *   - Read data bytes sequentially from base+8
	 *
	 * Use uncached direct mapping (TO_UNCAC) to access the device
	 * since ioremap may not be fully available this early.
	 */
	{
#define FWCFG_BASE 0x10080100UL
		volatile u16 __iomem *fwcfg_sel =
			(volatile u16 __iomem *)TO_UNCAC(FWCFG_BASE);
		volatile u8 __iomem *fwcfg_data =
			(volatile u8 __iomem *)TO_UNCAC(FWCFG_BASE + 8);
		unsigned int nr = 1;

		/*
		 * FW_CFG_NB_CPUS (selector 0x05): stored as LE u16.
		 * The control register is DEVICE_BIG_ENDIAN, so write the
		 * selector value in big-endian byte order.
		 * Data bytes are returned sequentially (byte 0 first = LSB).
		 */
		__raw_writew(cpu_to_be16(0x05), fwcfg_sel);
		nr = __raw_readb(fwcfg_data);
		nr |= (unsigned int)__raw_readb(fwcfg_data) << 8;
		if (nr == 0 || nr > NR_CPUS)
			nr = 1;

		/* FW_CFG_CPU_FREQ (selector 0x8001): stored as LE u64 */
		if (!cpu_clock_freq) {
			u64 freq = 0;
			int i;

			__raw_writew(cpu_to_be16(0x8001), fwcfg_sel);
			for (i = 0; i < 8; i++)
				freq |= (u64)__raw_readb(fwcfg_data) << (i * 8);
			if (freq)
				cpu_clock_freq = freq;
		}

		loongson_sysconf.nr_cpus = nr;
#undef FWCFG_BASE
	}

	loongson_sysconf.nr_nodes = (loongson_sysconf.nr_cpus +
				     loongson_sysconf.cores_per_node - 1) /
				    loongson_sysconf.cores_per_node;

	/* DMA defaults — Loongson-3A4000+ supports 48-bit DMA */
	loongson_sysconf.dma_mask_bits = 48;

	/* Bridge type: QEMU loongson3-virt uses VIRTUAL bridge */
	loongson_sysconf.bridgetype = VIRTUAL;
	loongson_sysconf.early_config = virtual_early_config;

	if (!cpu_clock_freq)
		cpu_clock_freq = 1800000000;

	pr_info("EFI: loongson_sysconf: PRID=%08x cputype=%d nr_cpus=%d "
		"cores_per_node=%d cpu_clock=%u\n",
		prid, loongson_sysconf.cputype, loongson_sysconf.nr_cpus,
		loongson_sysconf.cores_per_node, cpu_clock_freq);
}

/*
 * Merge the EFI stub's FDT (which contains /chosen with EFI properties)
 * with the built-in platform DTB (which contains device nodes).
 *
 * The EFI stub generates a minimal FDT with only /chosen containing:
 *   linux,uefi-system-table, linux,uefi-mmap-start, etc.
 * But it has no device nodes (no interrupt controllers, no UART, no PCI).
 * The kernel needs the built-in platform DTB for proper device support.
 *
 * Strategy: copy the built-in DTB into a static buffer, then copy
 * all properties from the EFI stub's /chosen into the buffer's /chosen.
 */
#define MERGED_DTB_SIZE (64 * 1024)
static u8 __initdata merged_dtb_buf[MERGED_DTB_SIZE] __aligned(8);

static void __init prom_efi_merge_dtb(void)
{
	void *efi_fdt = loongson_fdt_blob; /* EFI stub's FDT */
	void *plat_dtb; /* built-in platform DTB */
	int efi_chosen, merged_chosen, ret;
	int offset, len;
	const char *propname;
	const void *propval;

	/*
	 * Select the built-in platform DTB based on detected bridge type.
	 * For now, hardcode the virtual platform DTB since that's what
	 * QEMU loongson3-virt uses. In the future, this should detect the
	 * bridge type (LS7A, RS780E, VIRTUAL) like prom_lefi_init_env().
	 */
	plat_dtb = __dtb_loongson64v_4core_virtio_begin;

	/* Copy the platform DTB into our writable buffer */
	ret = fdt_open_into(plat_dtb, merged_dtb_buf, MERGED_DTB_SIZE);
	if (ret) {
		pr_err("EFI: fdt_open_into failed: %d\n", ret);
		return;
	}

	/* Find /chosen in the EFI stub FDT */
	efi_chosen = fdt_path_offset(efi_fdt, "/chosen");
	if (efi_chosen < 0) {
		pr_err("EFI: no /chosen in EFI stub FDT\n");
		return;
	}

	/* Find or create /chosen in the merged DTB */
	merged_chosen = fdt_path_offset(merged_dtb_buf, "/chosen");
	if (merged_chosen < 0) {
		merged_chosen = fdt_add_subnode(merged_dtb_buf, 0, "chosen");
		if (merged_chosen < 0) {
			pr_err("EFI: fdt_add_subnode(chosen) failed: %d\n",
			       merged_chosen);
			return;
		}
	}

	/*
	 * Copy all properties from EFI stub /chosen to merged /chosen.
	 * This includes linux,uefi-system-table, linux,uefi-mmap-*,
	 * bootargs, kaslr-seed, etc.
	 */
	fdt_for_each_property_offset(offset, efi_fdt, efi_chosen) {
		propval = fdt_getprop_by_offset(efi_fdt, offset, &propname,
						&len);
		if (!propval)
			continue;

		ret = fdt_setprop(merged_dtb_buf, merged_chosen, propname,
				  propval, len);
		if (ret)
			pr_err("EFI: fdt_setprop(%s) failed: %d\n", propname,
			       ret);
	}

	/* Switch to the merged DTB */
	loongson_fdt_blob = merged_dtb_buf;

	pr_info("EFI: merged platform DTB with EFI stub properties\n");
}

/*
 * Check if the kernel was booted via EFI stub by inspecting the FDT
 * for EFI-specific properties. The EFI stub installs "linux,uefi-system-table"
 * in /chosen before entering the kernel.
 */
static bool __init prom_is_efi_boot(void)
{
#ifdef CONFIG_EFI
	void *fdt;
	int chosen;

	if (fw_arg0 != (unsigned long)-2)
		return false;

	fdt = (void *)fw_arg2;
	if (!fdt || fdt_magic(fdt) != FDT_MAGIC)
		return false;

	chosen = fdt_path_offset(fdt, "/chosen");
	if (chosen < 0)
		return false;

	return fdt_getprop(fdt, chosen, "linux,uefi-system-table", NULL) != NULL;
#else
	return false;
#endif
}

void __init prom_init(void)
{
	fw_init_cmdline();

	if (fw_arg2 == 0 || (fdt_magic(fw_arg2) == FDT_MAGIC)) {
		loongson_sysconf.fw_interface = LOONGSON_DTB;
		prom_dtb_init_env();
	} else {
		loongson_sysconf.fw_interface = LOONGSON_LEFI;
		prom_lefi_init_env();
	}

	/* init base address of io space */
	set_io_port_base((unsigned long)PCI_IOBASE);

	if (loongson_sysconf.early_config)
		loongson_sysconf.early_config();

	/*
	 * When booting via EFI, skip the firmware-specific memory init.
	 * The EFI memory map (installed in the FDT by the EFI stub) will
	 * be processed later by efi_init() in setup_arch().
	 *
	 * We must also call early_init_dt_verify() here to set
	 * initial_boot_params, because efi_init() -> efi_get_fdt_params()
	 * reads EFI parameters from the FDT and needs initial_boot_params
	 * to be set. Normally __dt_setup_arch() sets it, but that runs
	 * later in plat_mem_setup(), which is after efi_init().
	 */
	if (prom_is_efi_boot()) {
		prom_efi_init_sysconf();
		prom_efi_merge_dtb();
		early_init_dt_verify(loongson_fdt_blob,
				     __pa(loongson_fdt_blob));

		/*
		 * Extract the EFI stub command line from FDT /chosen/bootargs
		 * and put it in arcs_cmdline so bootcmdline_init() picks it
		 * up.  Without this, CONFIG_MIPS_CMDLINE_FROM_BOOTLOADER
		 * never sees the EFI command line because fw_init_cmdline()
		 * doesn't parse firmware args in the EFI case (fw_arg0 == -2).
		 */
		{
			int chosen = fdt_path_offset(loongson_fdt_blob,
						     "/chosen");
			if (chosen >= 0) {
				int len;
				const char *p = fdt_getprop(loongson_fdt_blob,
							    chosen, "bootargs",
							    &len);
				if (p && len > 0)
					strscpy(arcs_cmdline, p,
						COMMAND_LINE_SIZE);
			}
		}
	} else {
#ifdef CONFIG_NUMA
		prom_init_numa_memory();
#else
		prom_init_memory();
#endif
	}

	/* Hardcode to CPU UART 0 */
	if ((read_c0_prid() & PRID_IMP_MASK) == PRID_IMP_LOONGSON_64R)
		setup_8250_early_printk_port(TO_UNCAC(LOONGSON_REG_BASE), 0, 1024);
	else
		setup_8250_early_printk_port(TO_UNCAC(LOONGSON_REG_BASE + 0x1e0), 0, 1024);

	register_smp_ops(&loongson3_smp_ops);
	board_nmi_handler_setup = mips_nmi_setup;
}

static int __init add_legacy_isa_io(struct fwnode_handle *fwnode, resource_size_t hw_start,
				    resource_size_t size)
{
	int ret = 0;
	struct logic_pio_hwaddr *range;
	unsigned long vaddr;

	range = kzalloc(sizeof(*range), GFP_ATOMIC);
	if (!range)
		return -ENOMEM;

	range->fwnode = fwnode;
	range->size = size = round_up(size, PAGE_SIZE);
	range->hw_start = hw_start;
	range->flags = LOGIC_PIO_CPU_MMIO;

	ret = logic_pio_register_range(range);
	if (ret) {
		kfree(range);
		return ret;
	}

	/* Legacy ISA must placed at the start of PCI_IOBASE */
	if (range->io_start != 0) {
		logic_pio_unregister_range(range);
		kfree(range);
		return -EINVAL;
	}

	vaddr = (unsigned long)PCI_IOBASE + range->io_start;

	vmap_page_range(vaddr, vaddr + size, hw_start, pgprot_device(PAGE_KERNEL));

	return 0;
}

static __init void reserve_pio_range(void)
{
	struct device_node *np;

	for_each_node_by_name(np, "isa") {
		struct of_range range;
		struct of_range_parser parser;

		pr_info("ISA Bridge: %pOF\n", np);

		if (of_range_parser_init(&parser, np)) {
			pr_info("Failed to parse resources.\n");
			of_node_put(np);
			break;
		}

		for_each_of_range(&parser, &range) {
			switch (range.flags & IORESOURCE_TYPE_BITS) {
			case IORESOURCE_IO:
				pr_info(" IO 0x%016llx..0x%016llx  ->  0x%016llx\n",
					range.cpu_addr,
					range.cpu_addr + range.size - 1,
					range.bus_addr);
				if (add_legacy_isa_io(&np->fwnode, range.cpu_addr, range.size))
					pr_warn("Failed to reserve legacy IO in Logic PIO\n");
				break;
			case IORESOURCE_MEM:
				pr_info(" MEM 0x%016llx..0x%016llx  ->  0x%016llx\n",
					range.cpu_addr,
					range.cpu_addr + range.size - 1,
					range.bus_addr);
				break;
			}
		}
	}
}

void __init arch_init_irq(void)
{
	reserve_pio_range();
	irqchip_init();
}
