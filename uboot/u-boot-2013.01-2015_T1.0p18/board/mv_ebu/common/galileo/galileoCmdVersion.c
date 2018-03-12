/*
 *    U-Boot for Microsemi SAS Switch
 *    Copyright (C) 2018 Microsemi Corporation
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; version 2 of the License.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *    NON INFRINGEMENT.  See the GNU General Public License for more details.
 */

#include <common.h>
#include <command.h>
#include <version.h>
#include <linux/compiler.h>
#ifdef CONFIG_SYS_COREBOOT
#include <asm/arch/sysinfo.h>
#endif

#include <net.h>
#include <config.h>
#include "mvCommon.h"
#include <environment.h>

#if defined(MV_INCLUDE_USB)
#include <usb.h>
#endif

#include <fs.h>

#define	DESTINATION_STRING	10

#if defined(CONFIG_CMD_NAND)
#include <nand.h>
extern nand_info_t nand_info[];       /* info for NAND chips */
#endif

#ifdef CONFIG_CMD_SF
#include <spi_flash.h>
extern struct spi_flash *flash;
#endif

#ifdef CONFIG_CMD_FLASH
#include <flash.h>
extern flash_info_t flash_info[];       /* info for FLASH chips */
#endif
#include "norflashlayout.h"
#include "../../../../../../build/vsm_version.h"

const char __weak version_string_basic[] = U_BOOT_VERSION_STRING;

void getBootRevisionNumber(char* uBootRevNumber)
{
   sprintf(uBootRevNumber, "%d.%d.%d.%d", BOOTLOADER_VERSION_MAJOR, BOOTLOADER_VERSION_MINOR, BOOTLOADER_VERSION_EXT_BUILD, BOOTLOADER_VERSION_INT_BUILD);
}

static int do_version(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
   printf("\n%s\n", version_string_basic);
   char  fwRev[64] = "";
   getBootRevisionNumber(fwRev);
   printf("U-Boot Revision Version: %s\n", fwRev);

#ifdef CC_VERSION_STRING
	puts(CC_VERSION_STRING "\n");
#endif
#ifdef LD_VERSION_STRING
	puts(LD_VERSION_STRING "\n");
#endif
#ifdef CONFIG_SYS_COREBOOT
	printf("coreboot-%s (%s)\n", lib_sysinfo.version, lib_sysinfo.build);
#endif
	return 0;
}

U_BOOT_CMD(
	version,	1,		1,	do_version,
	"print monitor, compiler and linker version",
	""
);
 
