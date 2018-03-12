/*
 * Power Management Service Unit(PMSU) support for Armada 370/XP platforms.
 *
 * Copyright (C) 2012 Marvell
 *
 * Yehuda Yitschak <yehuday@marvell.com>
 * Gregory Clement <gregory.clement@free-electrons.com>
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * The Armada 370 and Armada XP SOCs have a power management service
 * unit which is responsible for powering down and waking up CPUs and
 * other SOC units
 */

#define pr_fmt(fmt) "mvebu-pmsu: " fmt

#include <linux/clk.h>
#include <linux/cpu_pm.h>
#include <linux/cpu.h>
#include <linux/cpufreq-dt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mbus.h>
#include <linux/mvebu-v7-cpuidle.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/smp.h>
#include <linux/resource.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>
#include <asm/cp15.h>
#include <asm/smp_scu.h>
#include <asm/smp_plat.h>
#include <asm/suspend.h>
#include <asm/tlbflush.h>
#include "common.h"
#include "armada-370-xp.h"


#define PMSU_BASE_OFFSET    0x100
#define PMSU_REG_SIZE	    0x1000

/* PMSU MP registers */
#define PMSU_CONTROL_AND_CONFIG(cpu)	    ((cpu * 0x100) + 0x104)
#define PMSU_CONTROL_AND_CONFIG_DFS_REQ		BIT(18)
#define PMSU_CONTROL_AND_CONFIG_PWDDN_REQ	BIT(16)
#define PMSU_CONTROL_AND_CONFIG_L2_PWDDN	BIT(20)

#define PMSU_CPU_POWER_DOWN_CONTROL(cpu)    ((cpu * 0x100) + 0x108)

#define PMSU_CPU_POWER_DOWN_DIS_SNP_Q_SKIP	BIT(0)

#define PMSU_STATUS_AND_MASK(cpu)	    ((cpu * 0x100) + 0x10c)
#define PMSU_STATUS_AND_MASK_CPU_IDLE_WAIT	BIT(16)
#define PMSU_STATUS_AND_MASK_SNP_Q_EMPTY_WAIT	BIT(17)
#define PMSU_STATUS_AND_MASK_IRQ_WAKEUP		BIT(20)
#define PMSU_STATUS_AND_MASK_FIQ_WAKEUP		BIT(21)
#define PMSU_STATUS_AND_MASK_DBG_WAKEUP		BIT(22)
#define PMSU_STATUS_AND_MASK_IRQ_MASK		BIT(24)
#define PMSU_STATUS_AND_MASK_FIQ_MASK		BIT(25)

#define PMSU_EVENT_STATUS_AND_MASK(cpu)     ((cpu * 0x100) + 0x120)
#define PMSU_EVENT_STATUS_AND_MASK_DFS_DONE        BIT(1)
#define PMSU_EVENT_STATUS_AND_MASK_DFS_DONE_MASK   BIT(17)

#define PMSU_EVENT_STATUS_MSK(cpu)     ((cpu * 0x100) + 0x120)
#define PMSU_EVENT_STATUS_MSK_DFS_DONE        BIT(1)
#define PMSU_EVENT_STATUS_MSK_DFS_DONE_MASK   BIT(17)

#define PMSU_BOOT_ADDR_REDIRECT_OFFSET(cpu) ((cpu * 0x100) + 0x124)

/* PMSU fabric registers */
#define L2C_NFABRIC_PM_CTL		    0x4
#define L2C_NFABRIC_PM_CTL_PWR_DOWN		BIT(20)

/* PMSU delay registers */
#define PMSU_POWERDOWN_DELAY		    0xF04
#define PMSU_POWERDOWN_DELAY_PMU		BIT(1)
#define PMSU_POWERDOWN_DELAY_MASK		0xFFFE
#define PMSU_DFLT_ARMADA38X_DELAY	        0x64

/* CIB Control and Configuration */
#define CIB_CONTROL_AND_CONFIG_ACCK_EN_OFFSET		9
#define CIB_CONTROL_AND_CONFIG_ACCK_EN_MASK		0x1
#define CIB_CONTROL_AND_CONFIG_ACCK_EN_DISABLE		0x1
#define CIB_CONTROL_AND_CONFIG_ACCK_ENABLE		0x0
#define CIB_CONTROL_AND_CONFIG_EMPTY_STATUS_OFFSET	13
#define CIB_CONTROL_AND_CONFIG_EMPTY_STATUS_MASK	0x1
#define CIB_CONTROL_AND_CONFIG_NOT_EMPTY		0x0

/* CA9 MPcore SoC Control registers */

#define MPCORE_RESET_CTL		    0x64
#define MPCORE_RESET_CTL_L2			BIT(0)
#define MPCORE_RESET_CTL_DEBUG			BIT(16)

#define SRAM_PHYS_BASE  0xFFFF0000
#define BOOTROM_BASE    0xFFF00000
#define BOOTROM_SIZE    0x100000

#define ARMADA_370_CRYPT0_ENG_TARGET   0x9
#define ARMADA_370_CRYPT0_ENG_ATTR     0x1

/* PMSU DFS Core Sync in SMP */
#define PMSU_DFS_DELAY						1
#define PMSU_DFS_NO_SYNC					0
#define PMSU_DFS_SYNC						1
#define PMSU_DFS_SYNC_TIMEOUT				2
#define PMSU_DFS_SLAVE_TIMEOUT_THRESHOLD	20
#define PMSU_DFS_MASTER_TIMEOUT_THRESHOLD	4

extern void ll_disable_coherency(void);
extern void ll_enable_coherency(void);

extern void armada_370_xp_cpu_resume(void);
extern void armada_38x_cpu_resume(void);

void __iomem *scu_base;

static phys_addr_t pmsu_mp_phys_base;
static void __iomem *pmsu_mp_base;
static void __iomem *cib_control;

static spinlock_t pmsu_dfs_slave_lock;
static spinlock_t pmsu_dfs_master_lock;
static spinlock_t pmsu_dfs_sync_lock;
static struct cpumask cpu_dfs_mask;

static void *mvebu_cpu_resume;
static int (*mvebu_pmsu_dfs_request_ptr)(int cpu);

static void __iomem *sram_wa_virt_base[2];

static struct of_device_id of_pmsu_table[] = {
	{ .compatible = "marvell,armada-370-pmsu", },
	{ .compatible = "marvell,armada-370-xp-pmsu", },
	{ .compatible = "marvell,armada-380-pmsu", },
	{ /* end of list */ },
};

void mvebu_pmsu_set_cpu_boot_addr(int hw_cpu, void *boot_addr)
{
	writel(virt_to_phys(boot_addr), pmsu_mp_base +
		PMSU_BOOT_ADDR_REDIRECT_OFFSET(hw_cpu));
}

extern unsigned char mvebu_boot_wa_start;
extern unsigned char mvebu_boot_wa_end;
extern long sleep_save_sp[CONFIG_NR_CPUS];
/*
 * This function sets up the boot address workaround needed for SMP
 * boot on Armada 375 Z1 and cpuidle on Armada 370. It unmaps the
 * BootROM Mbus window, and instead remaps a crypto SRAM into which a
 * custom piece of code is copied to replace the problematic BootROM.
 */
int mvebu_setup_boot_addr_wa(unsigned int crypto_eng_target,
			     unsigned int crypto_eng_attribute,
			     phys_addr_t resume_addr_reg)
{
	void __iomem *sram_virt_base;
	u32 code_len = &mvebu_boot_wa_end - &mvebu_boot_wa_start;

	mvebu_mbus_del_window(BOOTROM_BASE, BOOTROM_SIZE);
	mvebu_mbus_add_window_by_id(crypto_eng_target, crypto_eng_attribute,
				    SRAM_PHYS_BASE, SZ_64K);

	sram_virt_base = ioremap(SRAM_PHYS_BASE, SZ_64K);
	if (!sram_virt_base) {
		pr_err("Unable to map SRAM to setup the boot address WA\n");
		return -ENOMEM;
	}

	memcpy(sram_virt_base, &mvebu_boot_wa_start, code_len);

	/*
	 * The last word of the code copied in SRAM must contain the
	 * physical base address of the PMSU register. We
	 * intentionally store this address in the native endianness
	 * of the system.
	 */
	__raw_writel((unsigned long)resume_addr_reg,
		     sram_virt_base + code_len - 4);

	iounmap(sram_virt_base);

	return 0;
}

/*
 * Due to a known ARM architecture bug which is related to the PCIe bus,
 * the L2 cache cannot be cleaned during the cpuidle suspend flow under active PCIe traffic.
 * Therefore we copy the cpuidle saved data to the CESA SRAM instead of cleaning it to the DRAM
 * and copy it to the DRAM when we return from the idle state
 */
int armada_38x_cpuidle_wa(void)
{
	u32 *tmp;
	unsigned int hw_cpu = cpu_logical_map(smp_processor_id());

	tmp = sram_wa_virt_base[hw_cpu];
	*tmp = virt_to_phys(&sleep_save_sp[hw_cpu]);	/* Save address */
	tmp++;
	*tmp = sleep_save_sp[hw_cpu];			/* Save pointer address */
	tmp++;
	*tmp = 0x2C;					/* Save size */
	tmp++;
	memcpy(tmp, sleep_save_sp[hw_cpu] + 0xC0000000, 0x2C); /* Copy from virtual address (mmu enabled) */
}

static int __init mvebu_v7_pmsu_init(void)
{
	struct device_node *np;
	struct resource res;
	int ret = 0;

	np = of_find_matching_node(NULL, of_pmsu_table);
	if (!np)
		return 0;

	pr_info("Initializing Power Management Service Unit\n");

	if (of_address_to_resource(np, 0, &res)) {
		pr_err("unable to get resource\n");
		ret = -ENOENT;
		goto out;
	}

	if (of_device_is_compatible(np, "marvell,armada-370-xp-pmsu")) {
		pr_warn(FW_WARN "deprecated pmsu binding\n");
		res.start = res.start - PMSU_BASE_OFFSET;
		res.end = res.start + PMSU_REG_SIZE - 1;
	}

	if (!request_mem_region(res.start, resource_size(&res),
				np->full_name)) {
		pr_err("unable to request region\n");
		ret = -EBUSY;
		goto out;
	}

	pmsu_mp_phys_base = res.start;

	pmsu_mp_base = ioremap(res.start, resource_size(&res));
	if (!pmsu_mp_base) {
		pr_err("unable to map registers\n");
		release_mem_region(res.start, resource_size(&res));
		ret = -ENOMEM;
		goto out;
	}

	cib_control = of_iomap(np, 1);
	if (!cib_control) {
		pr_err("unable to map registers\n");
		iounmap(pmsu_mp_base);
		ret = -ENOMEM;
		goto out;
	}

 out:
	of_node_put(np);
	return ret;
}

static void mvebu_v7_pmsu_enable_l2_powerdown_onidle(void)
{
	u32 reg;

	if (pmsu_mp_base == NULL)
		return;

	/* Enable L2 & Fabric powerdown in Deep-Idle mode - Fabric */
	reg = readl(pmsu_mp_base + L2C_NFABRIC_PM_CTL);
	reg |= L2C_NFABRIC_PM_CTL_PWR_DOWN;
	writel(reg, pmsu_mp_base + L2C_NFABRIC_PM_CTL);
}

enum pmsu_idle_prepare_flags {
	PMSU_PREPARE_NORMAL = 0,
	PMSU_PREPARE_DEEP_IDLE = BIT(0),
	PMSU_PREPARE_SNOOP_DISABLE = BIT(1),
};

/* No locking is needed because we only access per-CPU registers */
static int mvebu_v7_pmsu_idle_prepare(unsigned long flags)
{
	unsigned int hw_cpu = cpu_logical_map(smp_processor_id());
	u32 reg;

	if (pmsu_mp_base == NULL)
		return -EINVAL;

	/*
	 * Adjust the PMSU configuration to wait for WFI signal, enable
	 * IRQ and FIQ as wakeup events, set wait for snoop queue empty
	 * indication and mask IRQ and FIQ from CPU
	 */
	reg = readl(pmsu_mp_base + PMSU_STATUS_AND_MASK(hw_cpu));
	reg |= PMSU_STATUS_AND_MASK_CPU_IDLE_WAIT    |
	       PMSU_STATUS_AND_MASK_IRQ_WAKEUP       |
	       PMSU_STATUS_AND_MASK_FIQ_WAKEUP       |
	       PMSU_STATUS_AND_MASK_SNP_Q_EMPTY_WAIT |
	       PMSU_STATUS_AND_MASK_IRQ_MASK         |
	       PMSU_STATUS_AND_MASK_FIQ_MASK;
	writel(reg, pmsu_mp_base + PMSU_STATUS_AND_MASK(hw_cpu));

	reg = readl(pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(hw_cpu));
	/* ask HW to power down the L2 Cache if needed */
	if (flags & PMSU_PREPARE_DEEP_IDLE)
		reg |= PMSU_CONTROL_AND_CONFIG_L2_PWDDN;

	/* request power down */
	reg |= PMSU_CONTROL_AND_CONFIG_PWDDN_REQ;
	writel(reg, pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(hw_cpu));

	if (flags & PMSU_PREPARE_SNOOP_DISABLE) {
		/* Disable snoop disable by HW - SW is taking care of it */
		reg = readl(pmsu_mp_base + PMSU_CPU_POWER_DOWN_CONTROL(hw_cpu));
		reg |= PMSU_CPU_POWER_DOWN_DIS_SNP_Q_SKIP;
		writel(reg, pmsu_mp_base + PMSU_CPU_POWER_DOWN_CONTROL(hw_cpu));
	}

	return 0;
}

int armada_370_xp_pmsu_idle_enter(unsigned long deepidle)
{
	unsigned long flags = PMSU_PREPARE_SNOOP_DISABLE;
	int ret;

	if (deepidle)
		flags |= PMSU_PREPARE_DEEP_IDLE;

	ret = mvebu_v7_pmsu_idle_prepare(flags);
	if (ret)
		return ret;

	v7_exit_coherency_flush(all);

	ll_disable_coherency();

	dsb();

	wfi();

	/* If we are here, wfi failed. As processors run out of
	 * coherency for some time, tlbs might be stale, so flush them
	 */
	local_flush_tlb_all();

	ll_enable_coherency();

	/* Test the CR_C bit and set it if it was cleared */
	asm volatile(
	"mrc	p15, 0, r0, c1, c0, 0 \n\t"
	"tst	r0, #(1 << 2) \n\t"
	"orreq	r0, r0, #(1 << 2) \n\t"
	"mcreq	p15, 0, r0, c1, c0, 0 \n\t"
	"isb	"
	: : : "r0");

	pr_debug("Failed to suspend the system\n");

	return 0;
}

static int armada_370_xp_cpu_suspend(unsigned long deepidle)
{
	return cpu_suspend(deepidle, armada_370_xp_pmsu_idle_enter);
}

int armada_38x_do_cpu_suspend(unsigned long deepidle)
{
	unsigned long flags = 0;

	armada_38x_cpuidle_wa();

	if (deepidle)
		flags |= PMSU_PREPARE_DEEP_IDLE;

	mvebu_v7_pmsu_idle_prepare(flags);
	/*
	 * Already flushed cache, but do it again as the outer cache
	 * functions dirty the cache with spinlocks
	 */
	v7_exit_coherency_flush(louis);

	scu_power_mode(scu_base, SCU_PM_POWEROFF);

	cpu_do_idle();

	return 1;
}

static int armada_38x_cpu_suspend(unsigned long deepidle)
{
	return cpu_suspend(false, armada_38x_do_cpu_suspend);
}

/* No locking is needed because we only access per-CPU registers */
void mvebu_v7_pmsu_idle_exit(void)
{
	unsigned int hw_cpu = cpu_logical_map(smp_processor_id());
	u32 reg;

	if (pmsu_mp_base == NULL)
		return;
	/* cancel ask HW to power down the L2 Cache if possible */
	reg = readl(pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(hw_cpu));
	reg &= ~PMSU_CONTROL_AND_CONFIG_L2_PWDDN;
	/*
	 * When exiting from idle state such as cpuidle or hotplug,
	 * Enable PMU wait for the CPU to enter WFI when doing DFS
	 * by setting CPUx Frequency ID to 1
	 */
	reg |= (1 << 4);
	writel(reg, pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(hw_cpu));

	/* cancel Enable wakeup events and mask interrupts */
	reg = readl(pmsu_mp_base + PMSU_STATUS_AND_MASK(hw_cpu));
	reg &= ~(PMSU_STATUS_AND_MASK_IRQ_WAKEUP | PMSU_STATUS_AND_MASK_FIQ_WAKEUP);
	reg &= ~PMSU_STATUS_AND_MASK_CPU_IDLE_WAIT;
	reg &= ~PMSU_STATUS_AND_MASK_SNP_Q_EMPTY_WAIT;
	reg &= ~(PMSU_STATUS_AND_MASK_IRQ_MASK | PMSU_STATUS_AND_MASK_FIQ_MASK);
	writel(reg, pmsu_mp_base + PMSU_STATUS_AND_MASK(hw_cpu));
}

void mvebu_v7_pmsu_disable_dfs_cpu(int hw_cpu)
{
	u32 reg;
	if (pmsu_mp_base == NULL)
		return;
	/*
	 * Disable PMU wait for the CPU to enter WFI when doing DFS
	 * by setting CPUx Frequency ID to 0
	 */
	reg = readl(pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(hw_cpu));
	reg &= ~(0xf << 4);
	writel(reg, pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(hw_cpu));
}

static int mvebu_v7_cpu_pm_notify(struct notifier_block *self,
				    unsigned long action, void *hcpu)
{
	if (action == CPU_PM_ENTER) {
		unsigned int hw_cpu = cpu_logical_map(smp_processor_id());
		mvebu_pmsu_set_cpu_boot_addr(hw_cpu, mvebu_cpu_resume);
	} else if (action == CPU_PM_EXIT) {
		mvebu_v7_pmsu_idle_exit();
	}

	return NOTIFY_OK;
}

static struct notifier_block mvebu_v7_cpu_pm_notifier = {
	.notifier_call = mvebu_v7_cpu_pm_notify,
};

static struct mvebu_v7_cpuidle armada_370_cpuidle = {
	.type = CPUIDLE_ARMADA_370,
	.cpu_suspend = armada_370_xp_cpu_suspend,
};

static struct mvebu_v7_cpuidle armada_38x_cpuidle = {
	.type = CPUIDLE_ARMADA_38X,
	.cpu_suspend = armada_38x_cpu_suspend,
};

static struct mvebu_v7_cpuidle armada_xp_cpuidle = {
	.type = CPUIDLE_ARMADA_XP,
	.cpu_suspend = armada_370_xp_cpu_suspend,
};

static struct platform_device mvebu_v7_cpuidle_device = {
	.name = "cpuidle-mvebu-v7",
};

static __init int armada_370_cpuidle_init(void)
{
	struct device_node *np;
	phys_addr_t redirect_reg;

	np = of_find_compatible_node(NULL, NULL, "marvell,coherency-fabric");
	if (!np)
		return -ENODEV;
	of_node_put(np);

	/*
	 * On Armada 370, there is "a slow exit process from the deep
	 * idle state due to heavy L1/L2 cache cleanup operations
	 * performed by the BootROM software". To avoid this, we
	 * replace the restart code of the bootrom by a a simple jump
	 * to the boot address. Then the code located at this boot
	 * address will take care of the initialization.
	 */
	redirect_reg = pmsu_mp_phys_base + PMSU_BOOT_ADDR_REDIRECT_OFFSET(0);
	mvebu_setup_boot_addr_wa(ARMADA_370_CRYPT0_ENG_TARGET,
				 ARMADA_370_CRYPT0_ENG_ATTR,
				 redirect_reg);

	mvebu_cpu_resume = armada_370_xp_cpu_resume;
	mvebu_v7_cpuidle_device.dev.platform_data = &armada_370_cpuidle;

	return 0;
}

int armada_38x_cpuidle_init(void)
{
	struct device_node *np;
	void __iomem *mpsoc_base;
	u32 reg;

	np = of_find_compatible_node(NULL, NULL,
				     "marvell,armada-380-coherency-fabric");
	if (!np)
		return -ENODEV;
	of_node_put(np);

	np = of_find_compatible_node(NULL, NULL,
				     "marvell,armada-380-mpcore-soc-ctrl");
	if (!np)
		return -ENODEV;
	mpsoc_base = of_iomap(np, 0);
	BUG_ON(!mpsoc_base);
	of_node_put(np);

	/* Set up reset mask when powering down the cpus */
	reg = readl(mpsoc_base + MPCORE_RESET_CTL);
	reg |= MPCORE_RESET_CTL_L2;
	reg |= MPCORE_RESET_CTL_DEBUG;
	writel(reg, mpsoc_base + MPCORE_RESET_CTL);
	iounmap(mpsoc_base);

	/* Set up delay */
	reg = readl(pmsu_mp_base + PMSU_POWERDOWN_DELAY);
	reg &= ~PMSU_POWERDOWN_DELAY_MASK;
	reg |= PMSU_DFLT_ARMADA38X_DELAY;
	reg |= PMSU_POWERDOWN_DELAY_PMU;
	writel(reg, pmsu_mp_base + PMSU_POWERDOWN_DELAY);

	mvebu_cpu_resume = armada_38x_cpu_resume;
	mvebu_v7_cpuidle_device.dev.platform_data = &armada_38x_cpuidle;

	/* PCIe deadlock WA for Armada 38x cpuidle */
	/* CESA SRAM remap */
	sram_wa_virt_base[0] = ioremap(0xf1100000, SZ_64);
	sram_wa_virt_base[1] = ioremap(0xf1110000, SZ_64);

	/* Disable the L2 cache clean function as it is being used in the cpu_suspend
	 * Flow and violates the PCIe deadlock WA.
	 * We cannot disable this function in the L2 cache driver as it will break SMP boot */
	/* TBD - Disable cpuidle outer cache WA as it affects Suspend to RAM */
#if 0
	outer_cache.clean_range = NULL;
#endif
	return 0;
}

static __init int armada_xp_cpuidle_init(void)
{
	struct device_node *np;
	np = of_find_compatible_node(NULL, NULL, "marvell,coherency-fabric");
	if (!np)
		return -ENODEV;
	of_node_put(np);

	mvebu_cpu_resume = armada_370_xp_cpu_resume;
	mvebu_v7_cpuidle_device.dev.platform_data = &armada_xp_cpuidle;

	return 0;
}

static int __init mvebu_v7_cpu_pm_init(void)
{
	struct device_node *np;
	int ret;

	np = of_find_matching_node(NULL, of_pmsu_table);
	if (!np)
		return 0;
	of_node_put(np);

	if (of_machine_is_compatible("marvell,armadaxp"))
		ret = armada_xp_cpuidle_init();
	else if (of_machine_is_compatible("marvell,armada370"))
		ret = armada_370_cpuidle_init();
	else if (of_machine_is_compatible("marvell,armada38x"))
		ret = armada_38x_cpuidle_init();
	else
		return 0;

	if (ret)
		return ret;

	mvebu_v7_pmsu_enable_l2_powerdown_onidle();
	platform_device_register(&mvebu_v7_cpuidle_device);
	cpu_pm_register_notifier(&mvebu_v7_cpu_pm_notifier);

	return 0;
}

arch_initcall(mvebu_v7_cpu_pm_init);
early_initcall(mvebu_v7_pmsu_init);

static void mvebu_pmsu_dfs_request_local(void *data)
{
	u32 cpu;
	u32 reg;
	u32 slave_timestamp;
	u32 slave_sync_flag;
	u32 master_timestamp;
	u32 master_sync_flag;
	unsigned long flags;

	local_irq_save(flags);
	cpu = smp_processor_id();

	if ((num_online_cpus() != 1) && spin_trylock(&pmsu_dfs_slave_lock)) {

		/* Start SW sync point in SMP mode */
		/* =============================== */

		/* Set CPU mask under spinlock */
		spin_lock(&pmsu_dfs_sync_lock);
		cpumask_set_cpu(cpu, &cpu_dfs_mask);
		spin_unlock(&pmsu_dfs_sync_lock);

		/* Wait for master core to disable CIB */
		slave_sync_flag = PMSU_DFS_NO_SYNC;
		slave_timestamp = PMSU_DFS_SLAVE_TIMEOUT_THRESHOLD;
		do {
			mdelay(PMSU_DFS_DELAY);
			reg = readl(cib_control) >> CIB_CONTROL_AND_CONFIG_ACCK_EN_OFFSET
									  & CIB_CONTROL_AND_CONFIG_ACCK_EN_MASK;
			slave_timestamp--;
			if (slave_timestamp == 0) {
				slave_sync_flag = PMSU_DFS_SYNC_TIMEOUT;
				break;
			}
		} while (reg == CIB_CONTROL_AND_CONFIG_ACCK_ENABLE);

		/* Slave core timeout */
		if (slave_sync_flag == PMSU_DFS_SYNC_TIMEOUT) {
			spin_lock(&pmsu_dfs_sync_lock);
			/* Validate CPU mask
			*  In case CPU mask not set for both cores (meaning master has not started execution)
			*  Clear CPU mask, unlock slave spinlock, restore interrupt, and exit
			*  In case CPU mask set for both cores, master has started execution therefore continue
			*  slave execution
			*/
			if (!cpumask_equal(cpu_online_mask, &cpu_dfs_mask))
				cpumask_clear_cpu(cpu, &cpu_dfs_mask);
			else
				slave_sync_flag = PMSU_DFS_SYNC;
			spin_unlock(&pmsu_dfs_sync_lock);

			if (slave_sync_flag != PMSU_DFS_SYNC) {
				spin_unlock(&pmsu_dfs_slave_lock);
				local_irq_restore(flags);
				return;
			}
		}

		/* Release slave spinlock
		*  Take Master spinlock - wait for master to finish execution
		*  Release master spinlock - once taken release for next DFS interval
		*/
		spin_unlock(&pmsu_dfs_slave_lock);
		spin_lock(&pmsu_dfs_master_lock);
		spin_unlock(&pmsu_dfs_master_lock);

	} else {
		if (num_online_cpus() != 1) {

			/* Allocate master spinlock - block slave execution until CIB is disabled and empty */
			spin_lock(&pmsu_dfs_master_lock);

			/* Set CPU mask under spinlock */
			spin_lock(&pmsu_dfs_sync_lock);
			cpumask_set_cpu(cpu, &cpu_dfs_mask);
			spin_unlock(&pmsu_dfs_sync_lock);

			/* Wait for slave core to set CPU mask */
			master_sync_flag = PMSU_DFS_NO_SYNC;
			master_timestamp = PMSU_DFS_MASTER_TIMEOUT_THRESHOLD;
			do {
				mdelay(PMSU_DFS_DELAY);
				master_timestamp--;
				if (master_timestamp == 0) {
					master_sync_flag = PMSU_DFS_SYNC_TIMEOUT;
					break;
				}
			} while (!cpumask_equal(cpu_online_mask, &cpu_dfs_mask));

			/* Master core timeout */
			if (master_sync_flag == PMSU_DFS_SYNC_TIMEOUT) {
				/* Master core started execution at the time slave core has timeout
				*  master core will timeout and exit
				*/
				spin_lock(&pmsu_dfs_sync_lock);
				if (!cpumask_equal(cpu_online_mask, &cpu_dfs_mask))
					cpumask_clear_cpu(cpu, &cpu_dfs_mask);
				spin_unlock(&pmsu_dfs_sync_lock);
				/* Release master spinlock, restore interrupts, and exit */
				spin_unlock(&pmsu_dfs_master_lock);
				local_irq_restore(flags);
				return;
			}

		} else {
			/* Single core
			*  Allocate slave spinlock, incase already allocated it will fail
			*  If not allocated then allocate it, once allocated release it for next interval
			*/
			spin_trylock(&pmsu_dfs_slave_lock);
			spin_unlock(&pmsu_dfs_slave_lock);
		}

		/* Disable CIB Ack */
		reg = readl(cib_control);
		reg |= (CIB_CONTROL_AND_CONFIG_ACCK_EN_DISABLE <<
				CIB_CONTROL_AND_CONFIG_ACCK_EN_OFFSET);
		writel(reg, cib_control);

		/* Wait until CIB is empty */
		while
			(((readl(cib_control) >> CIB_CONTROL_AND_CONFIG_EMPTY_STATUS_OFFSET) &
				CIB_CONTROL_AND_CONFIG_EMPTY_STATUS_MASK) == CIB_CONTROL_AND_CONFIG_NOT_EMPTY);

		/* Release master spinlock - blocking slave execution */
		if (num_online_cpus() != 1)
			spin_unlock(&pmsu_dfs_master_lock);
	}

	/* Reached SW sync point in SMP mode */
	/* ================================= */

	/* Clear any previous DFS DONE event & Mask the DFS done interrupt */
	reg  = readl(pmsu_mp_base + PMSU_EVENT_STATUS_AND_MASK(cpu));
	reg &= ~PMSU_EVENT_STATUS_AND_MASK_DFS_DONE;
	reg |= PMSU_EVENT_STATUS_AND_MASK_DFS_DONE_MASK;
	writel(reg, pmsu_mp_base + PMSU_EVENT_STATUS_AND_MASK(cpu));

	/* Prepare to enter idle */
	reg  = readl(pmsu_mp_base + PMSU_STATUS_AND_MASK(cpu));
	reg |= PMSU_STATUS_AND_MASK_CPU_IDLE_WAIT |
		   PMSU_STATUS_AND_MASK_IRQ_MASK      |
		   PMSU_STATUS_AND_MASK_FIQ_MASK;
	writel(reg, pmsu_mp_base + PMSU_STATUS_AND_MASK(cpu));

	/* Request the DFS transition */
	reg  = readl(pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(cpu));
	reg |= PMSU_CONTROL_AND_CONFIG_DFS_REQ;
	writel(reg, pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(cpu));

	isb();
	wfi();

	if (num_online_cpus() != 1) {
		spin_lock(&pmsu_dfs_sync_lock);
		cpumask_clear(&cpu_dfs_mask);
		spin_unlock(&pmsu_dfs_sync_lock);
	}

	/*
	 * We're back from idle, the DFS transition has completed,
	 * clear the idle wait indication.
	 */
	reg  = readl(pmsu_mp_base + PMSU_STATUS_AND_MASK(cpu));
	reg &= ~PMSU_STATUS_AND_MASK_CPU_IDLE_WAIT;
	writel(reg, pmsu_mp_base + PMSU_STATUS_AND_MASK(cpu));

	/* Restore the DFS mask to its original state */
	reg  = readl(pmsu_mp_base + PMSU_EVENT_STATUS_AND_MASK(cpu));
	reg &= ~PMSU_EVENT_STATUS_AND_MASK_DFS_DONE_MASK;
	writel(reg, pmsu_mp_base + PMSU_EVENT_STATUS_AND_MASK(cpu));

	/* Set cpu ID to default */
	local_irq_restore(flags);
}

int armada_xp_pmsu_dfs_request(int cpu)
{
	unsigned long timeout;
	int hwcpu = cpu_logical_map(cpu);
	u32 reg;

	/* Trigger the DFS on the appropriate CPU */
	smp_call_function_single(cpu, mvebu_pmsu_dfs_request_local,
				 NULL, false);

	/* Poll until the DFS done event is generated */
	timeout = jiffies + HZ;
	while (time_before(jiffies, timeout)) {
		reg = readl(pmsu_mp_base + PMSU_EVENT_STATUS_AND_MASK(hwcpu));
		if (reg & PMSU_EVENT_STATUS_AND_MASK_DFS_DONE)
			break;
		udelay(10);
	}

	if (time_after(jiffies, timeout))
		return -ETIME;

	return 0;
}

int armada_380_pmsu_dfs_request(int cpu)
{
	/*
	 * Protect CPU DFS from changing the number of online cpus number during
	 * frequency transition by temporarily disable cpu hotplug
	 */
	cpu_hotplug_disable();

	/* Trigger the DFS on the appropriate CPU */
	on_each_cpu(mvebu_pmsu_dfs_request_local, NULL, false);

	cpu_hotplug_enable();

	return 0;
}


int mvebu_pmsu_dfs_request(int cpu)
{
	return mvebu_pmsu_dfs_request_ptr(cpu);
}

struct cpufreq_dt_platform_data armada_xp_cpufreq_dt_pd = {
	.independent_clocks = true,
};

struct cpufreq_dt_platform_data armada_380_cpufreq_dt_pd = {
	.independent_clocks = false,
};

static int mvebu_v7_pmsu_register_cpufreq(int cpu)
{
	struct device *cpu_dev;
	struct clk *clk;
	int ret;
	struct device_node *np;

	/*
	* registers the operating points
	* supported (which are the nominal CPU frequency and half of
	* it), and registers the clock notifier that will take care
	* of doing the PMSU part of a frequency transition.
	*/

	for_each_child_of_node(of_find_node_by_path("/cpus"), np)
		if (of_get_property(np, "clock-names", NULL))
			break;

	if (!np) {
		pr_err("failed to find cpufreq node\n");
		return -ENOENT;
	}

	cpu_dev = get_cpu_device(cpu);
	if (!cpu_dev) {
		pr_err("Cannot get CPU %d\n", cpu);
		return 0;
	}

	cpu_dev->of_node = np;

	clk = clk_get(cpu_dev, 0);
	if (!clk) {
		pr_err("Cannot get clock for CPU %d\n", cpu);
		return -ENODEV;
	}

	/*
	 * In case of a failure of dev_pm_opp_add(), we don't
	 * bother with cleaning up the registered OPP (there's
	 * no function to do so), and simply cancel the
	 * registration of the cpufreq device.
	 */
		ret = dev_pm_opp_add(cpu_dev, clk_get_rate(clk), 0);
	if (ret) {
		clk_put(clk);
		return ret;
	}

		ret = dev_pm_opp_add(cpu_dev, clk_get_rate(clk) / 2, 0);
	if (ret) {
		clk_put(clk);
		return ret;
	}

	return 0;
}

static int __init mvebu_v7_pmsu_cpufreq_init(void)
{
	struct device_node *np;
	struct resource res;
	int ret, cpu;

	/* init DFS spin lock */
	spin_lock_init(&pmsu_dfs_slave_lock);
	spin_lock_init(&pmsu_dfs_master_lock);
	spin_lock_init(&pmsu_dfs_sync_lock);
	cpumask_clear(&cpu_dfs_mask);

	/*
	 * In order to have proper cpufreq handling, we need to ensure
	 * that the Device Tree description of the CPU clock includes
	 * the definition of the PMU DFS registers. If not, we do not
	 * register the clock notifier and the cpufreq driver. This
	 * piece of code is only for compatibility with old Device
	 * Trees.
	 */
	np = of_find_compatible_node(NULL, NULL, "marvell,armada-xp-cpu-clock");
	if (!np)
		return 0;

	ret = of_address_to_resource(np, 1, &res);
	if (ret) {
		pr_warn(FW_WARN "not enabling cpufreq, deprecated armada-xp-cpu-clock binding\n");
		of_node_put(np);
		return 0;
	}

	of_node_put(np);

	/* register cpu clock for each cpu */
	for_each_possible_cpu(cpu) {
		ret = mvebu_v7_pmsu_register_cpufreq(cpu);
		if (ret)
			return ret;
	}

	if (of_machine_is_compatible("marvell,armada38x")) {

		if (num_online_cpus() == 1)
			mvebu_v7_pmsu_disable_dfs_cpu(1);

		mvebu_pmsu_dfs_request_ptr = armada_380_pmsu_dfs_request;
		platform_device_register_data(NULL, "cpufreq-dt", -1,
					&armada_380_cpufreq_dt_pd, sizeof(armada_380_cpufreq_dt_pd));
	} else if (of_machine_is_compatible("marvell,armadaxp")) {

		mvebu_pmsu_dfs_request_ptr = armada_xp_pmsu_dfs_request;
		platform_device_register_data(NULL, "cpufreq-dt", -1,
					&armada_xp_cpufreq_dt_pd, sizeof(armada_xp_cpufreq_dt_pd));

	} else
		return 0;

	return 0;
}

device_initcall(mvebu_v7_pmsu_cpufreq_init);
