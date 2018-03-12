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

#ifndef GPIO_H
#define GPIO_H
#include "mvTypes.h"

// MPP pins from Galileo Marvell IOM.pdf
#define GPIO_DATA_IN_REG_EXPANDER_INT_BIT 18
#define GPIO_DATA_IN_REG_IOM_SLOT 37
#define GPIO_DATA_IN_REG_MFG_MODE 46

MV_U8 gpioDataInRead(MV_U8 pin);
MV_U8 gpioIntRead(MV_U8 pin);
void gpioDataOutWrite(MV_U8 pin, int value);
MV_U8 readBayNumber(void);
MV_BOOL isExpanderReady(void);
MV_BOOL isManufacturingMode(void);
void setMPP0ToUART(void);
#endif // GPIO_H
