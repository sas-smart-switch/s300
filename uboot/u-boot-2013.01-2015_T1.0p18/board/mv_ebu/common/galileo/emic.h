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

 /*
 * @file emic.h
 * @brief Header file for emic.c  
 *  
 */

#ifndef EMIC_H
#define EMIC_H

#include "diagcodes.h"



// Function prototypes

int getSwitchInfo(uchar* bayNum);
void GetSendSwitchExpanderInfo(void);

#endif // EMIC_H
