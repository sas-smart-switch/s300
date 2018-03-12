Microsemi Smart Switch SDK 
Version 0.3.8.7 (March-12t-2018)

To build uboot: 
 a. cd uboot/u-boot-2013.01-2015_T1.0p18 
 b. ./build_uboot_by_microsemi.sh

To build linux kernel:
 a. cd linux/linux-3.10.70-2015_T1.1p9
 b. make zImage 
 c. make armada-380-microsemi.dtb

To build rootfs:
 a. cd rootfs/buildroot-2015.11-16.06.0
 b. make

