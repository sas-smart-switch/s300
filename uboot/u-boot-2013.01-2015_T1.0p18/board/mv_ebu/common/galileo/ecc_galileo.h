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

//memory address of error counter registers
#define SBE_Counter_Offset			0xf1001458
#define DBE_Counter_Offset			0xf100145C
#define ARMADA_38X_IOM_FRED_FAULT_GPIO		32

void SBE_output(void);
void DBE_output(void);

//gpio_rejath.h
