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

/**
 * @file emic.c 
 * @brief 
 */
#include "mvTypes.h"
#include "common.h"
#include <i2c.h>
#include "emic.h"
#include "gpio.h"
#include "expanderi2c.h"


// *****************************************************************
// Get Switch Info from CANMIC
// *****************************************************************
MV_BOOL getSwitchInfo(MV_U8* bayNumber)
{
   *bayNumber = readBayNumber();
   return TRUE;
}

void GetSendSwitchExpanderInfo(void)
{
   // Get Switch Bay Number and pass to switch expander
   MV_U8 switchBayNum = 0;

   if(isExpanderReady())
   {
      if(getSwitchInfo(&switchBayNum))
      {
         if(sendSwitchBayNumToExpander(switchBayNum))
         {
            printf("Switch Bay Number (%d) sent to Expander\n", switchBayNum);
         }
      }

      const MV_BOOL isManufactureMode = isManufacturingMode();
      printf("Manufacture mode %s detected. Sending control code to Phys.",
         (isManufactureMode ? "" : "not"));
      if (setupAllExpanderPhys(isManufactureMode))
      {
         printf("Success sending control code.\n");
      }
      else
      {
         printf("Could not send control block.\n");
      }
   }
}
