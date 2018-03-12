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
#include "ecc_galileo.h"
#include "mvOs.h"
#include "gpio.h"

void SBE_output(void)
{
	if (MV_REG_READ(SBE_Counter_Offset) > 0){
		printf("Single-Bit Errors Detected: 0x%08x\n",
				MV_REG_READ(SBE_Counter_Offset));
	}else{ 
		printf("No single-bit errors detected\n");
	}
}



void DBE_output(void)
{
	
	if (MV_REG_READ(DBE_Counter_Offset) > 0){
		printf("Double-Bit Errors Detected: 0x%08x\n",
				MV_REG_READ(DBE_Counter_Offset));
		//assert IOM_FRED_FAULT_GPIO
		gpioDataOutWrite(ARMADA_38X_IOM_FRED_FAULT_GPIO, 1);
	}else{
		printf("No double-bit errors detected\n");
	}
	
}

