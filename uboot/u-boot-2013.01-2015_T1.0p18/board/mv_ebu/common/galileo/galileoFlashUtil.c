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

#include "common.h"
#include <net.h>
#include <config.h>
#include <common.h>
#include "mvCommon.h"
#include <command.h>
#include <net.h>
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

int getActiveEMMCFromSPIFlash( void )
{
   int eMMCActivePartitionId = 0;
   norStatusFlags statusFlag = {0}; 
   MV_U32 ret = 0;
   if(!flash)
   {
      flash = spi_flash_probe(CONFIG_ENV_SPI_BUS, CONFIG_ENV_SPI_CS, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
      if (!flash) 
      {
         printf("Failed to probe SPI Flash\n");
         set_default_env("!spi_flash_probe() failed");
         ret = 1;
      }
   }

   if(0 == ret) 
   {
      ret = spi_flash_read(flash, SPI_FLASH_FLAG_PARTITION_OFFSET, sizeof(norStatusFlags), (unsigned char*) &statusFlag);
   }

   if(0 == ret) 
   {
      if(0 == statusFlag.activeFlag )
      {
         eMMCActivePartitionId  = EMMC_SPI_ROM_1_ID;
         printf("Active boot partition: /dev/mmcblk0p1\n");

      }
      else
      {
         eMMCActivePartitionId  = EMMC_SPI_ROM_2_ID;
         printf("Active boot partition: /dev/mmcblk0p2\n");
      }
   }
   return eMMCActivePartitionId;
}

void swapFlagsInBothROM(void)
{
   norStatusFlags statusFlag = {0}; 
   norStatusFlags statusFlagToUpdate = {0};
   MV_U32 ret = 0;
   int isNeedSwapFlag = 0;

   if(!flash)
   {
      flash = spi_flash_probe(CONFIG_ENV_SPI_BUS, CONFIG_ENV_SPI_CS, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
      if (!flash) 
      {
         printf("swapFlagsInBothROM failed to probe SPI flash\n");
         set_default_env("!spi_flash_probe() failed");
         ret = 1;
      }
   }

   if( 0 == ret ) 
   {
      ret = spi_flash_read(flash, SPI_FLASH_FLAG_PARTITION_OFFSET, sizeof(norStatusFlags), (unsigned char*) &statusFlag);
      if (0 == ret) 
      {  
         if((0 == statusFlag.activeFlag) &&  (1 == statusFlag.mostRecentFlashFlag))
         {
            statusFlagToUpdate.activeFlag = 1; 
            statusFlagToUpdate.mostRecentFlashFlag = 1;
            isNeedSwapFlag = 1;
            printf("Attempting to swap boot partition from /dev/mmcblk0p1 to /dev/mmcblk0p2 \n");
         }
         else if ((1 == statusFlag.activeFlag) &&  (0 == statusFlag.mostRecentFlashFlag))
         {
            statusFlagToUpdate.activeFlag = 0;
            statusFlagToUpdate.mostRecentFlashFlag = 0;
            isNeedSwapFlag = 1;
            printf("Attempting to swap boot partition from /dev/mmcblk0p2 to /dev/mmcblk0p1 \n");
         }
         else
         {
             // place holder 
         }
         if(isNeedSwapFlag)
         {
            #ifdef CONFIG_SPI_FLASH_PROTECTION
               printf("Unprotecting flash:");
               spi_flash_protect(flash, 0);
               printf("\t\t[Done]\n");
            #endif

            spi_flash_erase(flash, SPI_FLASH_FLAG_PARTITION_OFFSET, SPI_FLASH_ONE_SECTOR_SIZE); 
            ret = spi_flash_write(flash, SPI_FLASH_FLAG_PARTITION_OFFSET, sizeof(norStatusFlags), (const void *)&statusFlagToUpdate);

            #ifdef CONFIG_SPI_FLASH_PROTECTION
              printf("Protecting flash:");
	          spi_flash_protect(flash, 1);
	          printf("\t\t[Done]\n");
            #endif

            if(1==ret) 
            {
               printf("Failed to update boot flag\n");
            }
            else
            {
               printf("\nSucceeded to update boot flag\n");
            }
         }
      }
   }
} 

void buildBootCmdArg(char* bootArgRoot, char* bootCmdArg )
{
   int eMMCActivePartitionId = getActiveEMMCFromSPIFlash();
   char* eMMCPartitionOne = EMMC_PARTITION_ROOTFS_SPI_1;
   char* eMMCPartitionTwo = EMMC_PARTITION_ROOTFS_SPI_2;

   if( EMMC_SPI_ROM_1_ID == eMMCActivePartitionId) 
   {
      sprintf(bootArgRoot, "root=%s rw rootdelay=5", eMMCPartitionOne); 
      sprintf(bootCmdArg,  "mmc rescan; ext2load mmc 0:1 0x2000000  $image_name; ext2load mmc 0:1 0x1000000 $fdtfile; setenv bootargs $console $mtdparts $bootargs_root; bootz 0x2000000 - 0x1000000");
   }
   else if (EMMC_SPI_ROM_2_ID == eMMCActivePartitionId) 
   {
      sprintf(bootArgRoot, "root=%s rw rootdelay=5", eMMCPartitionTwo); 
      sprintf(bootCmdArg,  "mmc rescan; ext2load mmc 0:2 0x2000000  $image_name; ext2load mmc 0:2 0x1000000 $fdtfile; setenv bootargs $console $mtdparts $bootargs_root; bootz 0x2000000 - 0x1000000");

   }
   else 
   {
      printf("Error: Wrong eMMCActivePartitionId is found \n"); 
   }
 }
 
// *****************************************************************
// Display detailed help for netboot CLI command
// *****************************************************************
static void DisplayNetBootHelp(void)
{
   printf("Network maintenance tools\n");
   printf("netboot [option [args ...]]\n");
   printf("   help\n");
   printf("      Displays this detailed help information\n");
   printf("   all <server ip> <gateway ip>\n");
   printf("      Mount rootfs, kernel and application from the server at IP address serverip\n");
   printf("      Please run: dhcp command before run: netboot all <server ip> <gateway ip>\n");
   printf("   resetenv\n");
   printf("      Resets environment variables to default.\n");
   printf("\n");
}

static int ClearEnv(void)
{
   MV_U32 ret = 0;
   if(!flash) 
   {
      flash = spi_flash_probe(CONFIG_ENV_SPI_BUS, CONFIG_ENV_SPI_CS, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
      if (!flash)
      {
         printf("Failed to probe SPI Flash\n");
         set_default_env("!spi_flash_probe() failed");
         ret = 1;
      }
   }

   #ifdef CONFIG_SPI_FLASH_PROTECTION
	  printf("Unprotecting flash:");
	  spi_flash_protect(flash, 0);
	  printf("\t\t[Done]\n");
   #endif
	
   if(0 == ret)
   {
      printf("Env erasing 0x%x - 0x%x:", CONFIG_ENV_OFFSET_SPI, CONFIG_ENV_OFFSET_SPI + CONFIG_ENV_SIZE_SPI); 
      spi_flash_erase(flash, CONFIG_ENV_OFFSET_SPI, CONFIG_ENV_SIZE_SPI);
      printf("\t[Env erasing done]\n");
   }

   #ifdef CONFIG_SPI_FLASH_PROTECTION
	  printf("Protecting flash:");
	  spi_flash_protect(flash, 1);
	  printf("\t\t[Done]\n");
   #endif
   return ret;
}


static void SetRootFSEnvironment(const char * gatewayIp)
{
   setenv("gatewayip", gatewayIp);
   setenv("rootpath", "/rootfs");
   setenv("image_name", "/boot/zImage-linux-3.10.70-2015_T1.1p9-armada_mvebu_smp");
   setenv("fdtfile", "/boot/armada-38x-modular-linux-3.10.70-2015_T1.1p9.dtb");
   setenv("bootargs_root", "root=/dev/nfs  rw");
   setenv("netdev", "eth1");
   setenv("bootcmd", "setenv bootargs_end :${gatewayip}:${netmask}:Armada:$netdev:none; tftpboot ${loadaddr} ${rootpath}${image_name}; tftpboot $fdtaddr ${rootpath}${fdtfile}; setenv bootargs ${console} ${bootargs_root} nfsroot=${serverip}:/var/nfs${rootpath},nfsvers=3 ip=${ipaddr}:${serverip}${bootargs_end}; bootz 0x2000000 - $fdtaddr;");
   setenv("loadaddr", "0x2000000");
   setenv("console", "console=ttyS0,115200");
}


// *****************************************************************
// Perform the selected network  maintenance function 
// *****************************************************************
static int DoNetBoot(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
   int   rc = 0;
   switch (argc) 
   {
      case 2:
         if(0 == strcmp(argv[1], "help")) 
         {
            DisplayNetBootHelp();
         }
         else if(0 == strcmp(argv[1], "resetenv"))
         {
            int ret= ClearEnv();
            if(0 != ret)
            {
               printf("Clearing environment settings failed \n");
               rc = 1;
            }
            else
            {
               printf("Uboot environment variables have been cleared\n");
               printf("**** Please reset your uboot to make them effective *****\n");
            }
         }
         else
         {
            rc = 1;
            DisplayNetBootHelp();
         }
         break; 
      case 4:
         if(0 == strcmp(argv[1], "all")) 
         {
            setenv("serverip", argv[2]);
            printf(" starting ... FS \n");
            SetRootFSEnvironment(argv[3]);
            printf(" ending ... FS \n");

            if(saveenv()) 
            {
               rc = 1;
               printf("Error: could not save environment variables.\n");
            }
            printf("Done: saveenv().\n");

         }
         else
         {
            printf("Wrong arguments, please refer:\n");
            DisplayNetBootHelp( );
            rc = 1;
         }
         break;
      default:
         DisplayNetBootHelp();
   }

   return rc;
} //  DoNetBoot


U_BOOT_CMD (
   netboot, 8, 0, (void *)DoNetBoot,
   "Network maintenance tools\n",
   "netboot [option [args ...]]\n"
   "   help\n"
   "      Displays this detailed help information\n"
   "   all <server ip> <gateway ip>\n"
   "      Mount rootfs and kernel from the server at IP address serverip\n"
   "      Please run: dhcp command before run: netboot all <server ip> <gateway ip>\n"
   "   resetenv\n"
   "      Resets environment variables to default.\n"
   "\n"
);
