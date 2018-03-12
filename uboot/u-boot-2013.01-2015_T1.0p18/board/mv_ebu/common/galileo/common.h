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
#ifndef GALILEO_COMMON
#define GALILEO_COMMON

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <stdio_dev.h>
#include <version.h>
#include <net.h>
#include <serial.h>
#include <nand.h>
#include <onenand_uboot.h>
#include <mmc.h>
#include <libfdt.h>
#include <fdtdec.h>
#include <post.h>
#include <logbuff.h>

DECLARE_GLOBAL_DATA_PTR;

#ifndef TRUE
#define TRUE   1
#endif
#ifndef FALSE
#define FALSE  0
#endif

// Memory test enumerations
#define MEM_TEST_STANDARD  0
#define MEM_TEST_EXTENDED  1
#define MEM_TEST_CLEAR     2
#define MEM_TEST_NONE      3

#define MEGABYTE           (1024 * 1024)
#define MEM_UPDATE_SIZE    MEGABYTE
#define BYTES_TO_MB(bytes) (bytes/MEGABYTE)
#define GIGABYTE        1024*MEGABYTE
//#define _64K            65535
#define BYTES_TO_64K(bytes) (bytes/_64K)

#endif
