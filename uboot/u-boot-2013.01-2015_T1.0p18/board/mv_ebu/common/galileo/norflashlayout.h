/////////////////////////////////////////////////////////////////////////////////////////
// This file is subject to the terms and conditions of the GNU General Public  Licence.//
// Copyright (C) 2018 Microsemi Corporation                                            //
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef SPI_FLASH_LAYOUT_H
#define SPI_FLASH_LAYOUT_H

/* In  the case of NOR only architecture, each redundant NOR Flash carried its
   own flag partition(we had one flag partition per Redundant ROM). Whereas,
   with NOR + eMMC architecture, both eMMC partitions are managed with single
   flags partition. Refer to norStatusFlags structure below */

#define SPI_ENTIRE_IMAGE_SIZE    0x00400000 // 4MB

// size here means reserved size(not the actual)
#define SPI_UBOOT_OFFSET         0x00000000
#define SPI_UBOOT_SIZE           0x001C0000 // 2MB - 256KB

#define SPI_CI_TOC_OFFSET        0x001C0000
#define SPI_CI_TOC_SIZE          0x00020000 // 128KB

#define SPI_FLAGS_OFFSET         0x001E0000
#define SPI_FLAGS_SIZE           0x00020000 // 128KB

#define SPI_UBOOT_COPY2_OFFSET   0x00200000
#define SPI_UBOOT_COPY2_SIZE     0x001C0000 // 2MB - 256KB

#define SPI_RESERVED_OFFSET      0x003C0000
#define SPI_RESERVED_SIZE        0x00040000 // 256KB

#define EMMC_PARTITION_ROOTFS_SPI_1  "/dev/mmcblk0p1"
#define EMMC_PARTITION_ROOTFS_SPI_2  "/dev/mmcblk0p2"
#define EMMC_SPI_ROM_1_ID  1
#define EMMC_SPI_ROM_2_ID  2
#define SPI_FLASH_FLAG_PARTITION_OFFSET 0x001E0000
#define SPI_FLASH_ONE_SECTOR_SIZE       0x00010000


// Bump it up everytime ROM layout changes
#define CURRENT_ROM_LAYOUT_VERSION     1

// Structure representing first dword of flag partition of composite image
typedef struct
{
   // keeping total structure size of 256 bytes

   /* "activeFlag" is a boolean: 0 means partition 1 active, 1 means partition 2 active
      "mostRecentFlashFlag" is a boolean: 0 means partition 1 recently flashed, 1 means partition 2 */
   unsigned int activeFlag : 1;           // 0 = eMMC partition 1 active, 1 = eMMC partition 2 active
   unsigned int mostRecentFlashFlag : 1;  // 0 = eMMC partition 1 recently flashed, 1 = eMMC partition 2
   unsigned int reserved : 30;            // Reserved for future use
   unsigned int romLayoutVersion;         // Version of ROM layout with the current build

   // if defining more fields in future reduce reserved by that much
   // as we don't keep any meta data for eMMC partitions in eMMC itself
   // it is likely we will put eMMC partition versions here in the future 
   unsigned char reserved2[248];
}norStatusFlags;

#endif
