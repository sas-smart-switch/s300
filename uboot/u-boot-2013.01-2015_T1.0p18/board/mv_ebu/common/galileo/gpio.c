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


#include "mvTypes.h"
#include "gpp/mvGppRegs.h"
#include "mvOs.h"
#include "common.h"
#include "gpio.h"

/* Read a DATA IN gpip pin value */
MV_U8 gpioDataInRead(MV_U8 pin)
{
   MV_U32 reg = 0;
   MV_U8 grp = pin/32;
   MV_U32 shift = pin % 32;
   MV_U32 value = 0;

   reg = MV_REG_READ(GPP_DATA_IN_REG(grp));
   value = (reg >> shift) & 0x1;
   
   printf("gpioDataInRead pin:%d, grp:%d, reg:0x%08X, sft:%d, value:%d\n", pin, grp, reg, shift, value);

   return value;
}

/* Read an interrupt gpip pin value */
MV_U8 gpioIntRead(MV_U8 pin)
{
   MV_U32 reg = 0;
   MV_U8 grp = pin/32;
   MV_U32 shift = pin % 32;
   MV_U32 value = 0;

   reg = MV_REG_READ(GPP_INT_CAUSE_REG(grp));
   value = (reg >> shift) & 0x1;
   
   printf("gpioIntRead pin:%d, grp:%d, reg:0x%08X, sft:%d, value:%d\n", pin, grp, reg, shift, value);

   return value;
}

/* Write a DATA OUT gpip pin value */
void gpioDataOutWrite(MV_U8 pin, int value)
{
    MV_U32 reg = 0;
    MV_U8 grp = pin/32;
    MV_U32 shift = pin % 32;

    reg = MV_REG_READ(GPP_DATA_OUT_REG(grp));
    reg &= ~(0x1 << shift);
    reg |= (value << shift);

    MV_REG_WRITE(GPP_DATA_OUT_REG(grp),reg);

}
MV_U8 readBayNumber(void)
{
   MV_U8 pinValue = gpioDataInRead(GPIO_DATA_IN_REG_IOM_SLOT);

   return pinValue + 1;
}

// *****************************************************************
// isExpanderReady()
//    If expander interrupt is pending, it is ready. If not,
//    try checking every 100ms for 10s and exit as not ready
// *****************************************************************
MV_BOOL isExpanderReady(void)
{
   MV_BOOL status = TRUE;

   static const MV_U32 EXP_INT_POLL_INTERVAL = 100000; // 100ms(converted to us)
   static const MV_U32 EXP_READY_RETRY = 30; // 30 retries
   static const MV_U32 EXP_READY_WAIT_TIME = 1000000; // 1 seconds

   if(!gpioIntRead(GPIO_DATA_IN_REG_EXPANDER_INT_BIT))
   {
      MV_U32 retry = EXP_READY_RETRY;

      // Wait for 10 sec for expander interrupt to be set, poll every 100ms
      while(retry && (!gpioIntRead(GPIO_DATA_IN_REG_EXPANDER_INT_BIT)))
      {
         udelay(EXP_INT_POLL_INTERVAL);
         retry--;
      }
      if(!retry)
      {
         printf("isExpanderReady: expander ready wait timed out \n");
         status = FALSE;
      }
      else
      {
         printf("isExpanderReady: expander is ready\n");

         //More delay for gSEP slave i2c task
         udelay(EXP_READY_WAIT_TIME);
      }
   }

   return status;
}

// *****************************************************************
// isManufacturingMode()
//    Checks if manufacture mode is enabled. 
// *****************************************************************
MV_BOOL isManufacturingMode(void)
{
   MV_BOOL status = FALSE;

   if(gpioDataInRead(GPIO_DATA_IN_REG_MFG_MODE) == 0)
   {
       status = TRUE;
   }

   return status;
}

// *****************************************************************
// setMPP0ToUART(void)
//    reverse  gpio value setting
//    default value is 0 - GPIO 
//    1 is UART - only applies to factory Mode 
// *****************************************************************

void setMPP0ToUART(void)
{
    MV_U32 MPP_SET_MASK_MPP0= 0x1;
    MV_U32 regData = ( MV_REG_READ(MPP_CONTROL_REG(0)) | (MPP_SET_MASK_MPP0));
    MV_REG_WRITE(MPP_CONTROL_REG(0), regData);
}



