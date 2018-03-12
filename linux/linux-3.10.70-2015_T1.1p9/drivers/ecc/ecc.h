
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

#define ECC_READ_REG(reg)                               ioread32(ecc->regbase_ecc + reg)
#define ECC_WRITE_REG(val, reg)                         { iowrite32(val, ecc->regbase_ecc + reg); }

#define DRAM_ECC_DDR_INTERRUPT_CONTROL_REG              0x54
/*error count registers*/
#define DRAM_ECC_SBE_COUNT_REG          		0x58
#define DRAM_ECC_DBE_COUNT_REG				0x5C
/*interrupt cause registers*/
#define DRAM_ECC_DDR_INTERRUPT_CAUSE_REG		0xD0
#define DRAM_ECC_DDR_MESSAGE_INTERRUPT_CAUSE_REG	0xD8
/*interrupt mask registers*/
#define DRAM_ECC_DDR_INTERRUPT_MASK_REG                 0xD4
#define DRAM_ECC_DDR_MESSAGE_INTERRUPT_MASK_REG         0xDC
/*asserting GPIO*/
#define ARMADA_38X_FMP_PART_RST_RQST			28
#define ARMADA_38X_IOM_FRED_FAULT_GPIO			32
/*writing to persistent storage*/
#define FRED_SLAVE_ADDRESS				0x32
#define ECC_SINGLE_BIT					0x01
#define ECC_DOUBLE_BIT					0x02
#define FRED_UPPER_8BIT					0x78
#define FRED_LOWER_8BIT					0x20

struct ecc_device{
    struct device dev;
	struct module *owner;
	int id;
	unsigned long irq_data;
};

//static int armada_ecc_read_error_from_perst_storage(void);
//static int armada_ecc_write_error_to_perst_storage(char type_of_error,int number_of_errors);
static irqreturn_t armada_ecc_irq_handler(int irq, void *ecc_ptr);
static int armada_ecc_probe(struct platform_device *pdev);
static int __exit armada_ecc_remove(struct platform_device *pdev);
static int __init armada_ecc_init(void);
static void __exit armada_ecc_exit(void);



