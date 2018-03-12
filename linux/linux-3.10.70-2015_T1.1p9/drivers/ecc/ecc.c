/*
 * ECC Driver for Marvell SOC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/module.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include "ecc.h"


typedef struct armada_ecc_s {
	struct ecc_device *ecc_dev;
	void __iomem      *regbase_ecc;
	void __iomem      *regbase_soc;
	uint32_t           irq;
} armada_ecc_t;


#if 0
static int armada_ecc_read_error_from_perst_storage(void)
{
	int ret;
	struct i2c_adapter *adap = i2c_get_adapter(0);
	struct i2c_msg msg;
	char buf[6] = {0x78,0x20,0x00,0x00,0x20,0x78};
	int count = 6;

	msg.addr = 0x32;
	msg.flags = msg.flags & I2C_M_TEN;
	msg.len = count;
	msg.flags |= I2C_M_RD;
	msg.buf = (char *)&buf;
	ret = i2c_transfer(adap, &msg, 1);

	/*
	 * If everything went ok (i.e. 1 msg transmitted), return #bytes
	 * transmitted, else error code.
	 */
	return (ret == 1) ? count : ret;
}

static int armada_ecc_write_error_to_perst_storage(char type_of_error,int number_of_errors)
{
	int ret;
	struct i2c_adapter *adap = i2c_get_adapter(0);
	struct i2c_msg msg;
	char buf[4] = {FRED_LOWER_8BIT,FRED_UPPER_8BIT,type_of_error,number_of_errors};
	int count = 4;

	msg.addr = FRED_SLAVE_ADDRESS;
	msg.flags = 0;
	msg.len = count;
	msg.buf = buf;

	ret = i2c_transfer(adap, &msg, 1);
	/*
	 * If everything went ok (i.e. 1 msg transmitted), return #bytes
	 * transmitted, else error code.
	 */
	return (ret == 1) ? count : ret;
}
#endif

static irqreturn_t armada_ecc_irq_handler(int irq, void *ecc_ptr)
{
	armada_ecc_t *ecc = (armada_ecc_t *)ecc_ptr;
	irqreturn_t irq_status = IRQ_NONE;

	/*check count registers for any present errors*/
	if (ECC_READ_REG(DRAM_ECC_SBE_COUNT_REG) > 0x01){
		printk("armada_ecc: Single-Bit Errors Detected: 0x%08x\n",
				ECC_READ_REG(DRAM_ECC_SBE_COUNT_REG));
	}


	if (ECC_READ_REG(DRAM_ECC_DBE_COUNT_REG) > 0x0)	{
		printk("armada_ecc: Double-Bit Errors Detected: 0x%08x\n",
				ECC_READ_REG(DRAM_ECC_DBE_COUNT_REG));
        //In case of double bit, crash the linux kernel
		ECC_WRITE_REG(0,DRAM_ECC_DBE_COUNT_REG);
        panic("armada_ecc: ECC- Uncorrectable double bit ecc error reported by DRAM controller");
	}
	
	if (ECC_READ_REG(DRAM_ECC_DDR_INTERRUPT_CAUSE_REG) & 0x01)
		printk("armada_ecc: DDR Cause Register: 0x%08x\n",
				ECC_READ_REG(DRAM_ECC_DDR_INTERRUPT_CAUSE_REG));

	if (ECC_READ_REG(DRAM_ECC_DDR_INTERRUPT_CAUSE_REG) & 0x02)
		printk("armada_ecc: DDR Cause Register: 0x%08x\n",
				ECC_READ_REG(DRAM_ECC_DDR_INTERRUPT_CAUSE_REG));

		
	ECC_WRITE_REG(0,DRAM_ECC_SBE_COUNT_REG);
	ECC_WRITE_REG(0,DRAM_ECC_DDR_INTERRUPT_CAUSE_REG);
	ECC_WRITE_REG(0,DRAM_ECC_DDR_MESSAGE_INTERRUPT_CAUSE_REG);

	irq_status = IRQ_HANDLED;
	return irq_status;
}

static int armada_ecc_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;
	armada_ecc_t *ecc = kzalloc(sizeof(struct armada_ecc_s), GFP_KERNEL);
	
	if (unlikely(!ecc)) {
		ret = -ENOMEM;
		goto exit;
	} else
		platform_set_drvdata(pdev, ecc);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(res == NULL)) {
		dev_err(&pdev->dev, "No IO resource\n");
		ret = -ENOENT;
		goto errExit1;
	}

	ecc->regbase_ecc = ioremap(res->start, resource_size(res));
	if (unlikely(!ecc->regbase_ecc)) {
		dev_err(&pdev->dev, "No REMAP resource\n");
		ret = -EINVAL;
		goto errExit1;
	}

	ecc->irq = platform_get_irq(pdev, 0);
	if (unlikely(ecc->irq <= 0)) {
		dev_err(&pdev->dev, "No IRQ resource\n");
		ret = -ENOENT;
		goto errExit2;
	}

	ret = request_irq(ecc->irq, armada_ecc_irq_handler, 0, "armada-ecc", ecc);
	if (unlikely(ret)) {
		dev_err(&pdev->dev, "Request IRQ failed with %d, IRQ %d\n", ret, ecc->irq);
		ret = ret;
	} else {
		/*Setting single bit threshold to 255 :Bit 16-23*/
		ECC_WRITE_REG(0x800000,DRAM_ECC_DDR_INTERRUPT_CONTROL_REG);
		/*enabling interrupt mask registers*/
		ECC_WRITE_REG(0x1,DRAM_ECC_DDR_INTERRUPT_MASK_REG);
		ECC_WRITE_REG(0x1,DRAM_ECC_DDR_MESSAGE_INTERRUPT_MASK_REG);

		printk("armada_ecc: Error Control Register: 0x%08x\n",
					ECC_READ_REG(DRAM_ECC_DDR_INTERRUPT_CONTROL_REG));
		printk("armada_ecc: Error Interrupt Mask Register: 0x%08x\n",
					ECC_READ_REG(DRAM_ECC_DDR_INTERRUPT_MASK_REG));
		printk("armada_ecc: Message Interrupt Mask Register: 0x%08x\n",
					ECC_READ_REG(DRAM_ECC_DDR_MESSAGE_INTERRUPT_MASK_REG));

		goto exit;
	}

	free_irq(ecc->irq, ecc);

errExit2:
	iounmap(ecc->regbase_ecc);

errExit1:
	kfree(ecc);

exit:
	return ret;
}

static int __exit armada_ecc_remove(struct platform_device *pdev)
{
	armada_ecc_t *ecc = platform_get_drvdata(pdev);
	printk("armada_ecc: %s function.\n",__func__);
/*disabling interrupt mask registers*/
	printk("armada_ecc: Interrupt Mask Registers Disabled\n");
	ECC_WRITE_REG(0x0,DRAM_ECC_DDR_INTERRUPT_CONTROL_REG);
	ECC_WRITE_REG(0,DRAM_ECC_DDR_INTERRUPT_MASK_REG);
	ECC_WRITE_REG(0,DRAM_ECC_DDR_MESSAGE_INTERRUPT_MASK_REG);
		
	if (ecc) {
		free_irq(ecc->irq, ecc);
		iounmap(ecc->regbase_ecc);
		kfree(ecc);
	}

	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id ecc_armada_of_match_table[] = {
	{ .compatible = "marvell,armada-xp-sdram-controller", },
	{}
};
#endif

static struct platform_driver armada_ecc_driver = {
	.probe      = armada_ecc_probe,
	.remove     = __exit_p(armada_ecc_remove),
	.driver     = {
		.name   = "armada-ecc",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(ecc_armada_of_match_table),
	},
};

static int __init armada_ecc_init(void)
{
	printk("armada_ecc: %s function.\n",__func__);
	return platform_driver_register(&armada_ecc_driver);
}

static void __exit armada_ecc_exit(void)
{
	printk("armada_ecc: %s function.\n",__func__);
	platform_driver_unregister(&armada_ecc_driver);
}

module_init(armada_ecc_init);
module_exit(armada_ecc_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("opensource@dell.com>");
MODULE_DESCRIPTION("Marvell DRAM ECC");

