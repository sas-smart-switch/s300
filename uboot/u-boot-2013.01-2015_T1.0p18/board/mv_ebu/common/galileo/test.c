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
#include "gpio.h"
#include "expanderi2c.h"
#include "diagcodes.h"

#include <i2c.h>
#include <linux/ctype.h>

#define PHY_MAP_STR_LENGTH 16
#define TOLOWER(x) ((x) | 0x20) // for digits and letters only.

int showGpioPin(char pin);
int showBayNumber(void);
int sendBayNumber(void);
int sendPhyMap(const char *map);
int ReadExpanderInterrupt(void);

static void DisplayDiagsHelp (void)
{
   printf ("Diagnostic tests\n");
   printf ("[option [args ...]]\n");
   printf ("   allhelp\n");
   printf ("      Display this detailed help information\n");
   printf ("   readgpio\n");
   printf ("      Read a GPIO pin value\n");
   printf ("   baynumber\n");
   printf ("      Show switch bay number\n");
   printf ("   sendbaynumber\n");
   printf ("      Send switch bay number to expander\n");
   printf ("   read_exp_int\n");
   printf ("      Read the expander interrupt status\n");
   printf ("   sendphymap\n");
   printf ("      Send Phy map: sendphymap ffffffffffffffff\n");

   printf ("\n");
} // DisplayDiagsHelp


// *****************************************************************
// Diags main routine
// *****************************************************************
static int DoDiags (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]) 
{
   int   rc = 0;

   if (argc == 1) 
      printf("cmdtp->help length: %u\n", strlen(cmdtp->help) + 1);

   if (argc == 1) 
   {
      DisplayDiagsHelp();
   }
   else if (argc == 2)
   {
      if (strcmp (argv[1], "allhelp") == 0) 
      {
         DisplayDiagsHelp();
      } 
      else if (strcmp (argv[1], "baynumber") == 0) 
      {
         rc = showBayNumber();
      }
      else if (strcmp (argv[1], "sendbaynumber") == 0) 
      {
         rc = sendBayNumber();
      }
      else if (strcmp(argv[1], "read_exp_int") == 0) 
      {
         rc = ReadExpanderInterrupt();
      }
   }
   else if (argc == 3) {
      if (strcmp(argv[1], "readgpio") == 0) {
         rc = showGpioPin(simple_strtoul(argv[2], NULL, 10));
      }
      else if (strcmp(argv[1], "sendphymap") == 0) {
         rc = sendPhyMap(argv[2]);
      }
   }
   else if (argc == 4) {
   }

   printf("RC: 0x%08X\n", rc);
   return rc;

} // DoDiags

U_BOOT_CMD (
   diags, 4, 1, (void *)DoDiags,
   "diags   - diagnostic tests\n",
// U-boot locks up or crashes when the help string is > 1020.
// The current size of the actual diags help is far too large to
// fit within that limit, so a separate command is required.
   "The diags command tests the SAS switch hardware.\n"
   "Type 'diags allhelp' to see detailed descriptions of the available tests.\n"
);

int showGpioPin(char pin)
{
   char value = gpioDataInRead(pin);
   printf("GPIO pin %d value = %d\n", pin, value);

   return DIAG_RC_NONE;
}

int showBayNumber(void)
{
   char bayNumber = readBayNumber();
   printf("Bay number: %d\n", bayNumber);

   return DIAG_RC_NONE;
}

int sendBayNumber(void)
{
   int rc = 0;
   char bayNumber = readBayNumber();

   rc = sendSwitchBayNumToExpander(bayNumber);

   if(rc)
   {
      printf("Send bay number %d to expander succeeded\n", bayNumber);
   }
   else
   {
      printf("Send bay number %d to expander failed\n", bayNumber);
   }

   return DIAG_RC_NONE;
}

int verifyPhyMap(const char *map)
{
   int rc = DIAG_RC_ILLEGAL_ARGUMENT;

   if (map)
   {
      size_t len = strnlen(map, PHY_MAP_STR_LENGTH);
      if (len == PHY_MAP_STR_LENGTH && map[len] == 0)
      {
         int allOk = 1;

         for(int n=0;n<len && allOk;++n)
         {
            if (!isxdigit(map[n]))
            {
               allOk = 0;
            }
         }

         if (allOk)
         {
            rc = DIAG_RC_NONE;
         }
      }
   }

   return rc;
}

#define HexDigitToL(x) ((x >= '0' && x <= '9') ? (x - '0') : (x - 'a' + 0xa))

int sendPhyMap(const char *map)
{
   int rc = verifyPhyMap(map);

   if (rc == DIAG_RC_NONE)
   {
      struct ExpanderPortPhyControlBlock writeData = {0};

      // Fill the phy control block to be written
      writeData.BlockNum = EXPANDER_PORT_PHY_CONTROL_BLK;
      /* Transfer only the port enables that apply to Peabody.
         Canmic block and expander block port order are not the same */

      writeData.Port0 = (uchar)HexDigitToL(map[0]);
      writeData.Port1 = (uchar)HexDigitToL(map[1]);
      writeData.Port2 = (uchar)HexDigitToL(map[2]);
      writeData.Port3 = (uchar)HexDigitToL(map[3]);
      writeData.Port4 = (uchar)HexDigitToL(map[4]);
      writeData.Port5 = (uchar)HexDigitToL(map[5]);
      writeData.Port6 = (uchar)HexDigitToL(map[6]);
      writeData.Port7 = (uchar)HexDigitToL(map[7]);
      writeData.Port8 = (uchar)HexDigitToL(map[8]);
      writeData.Port9 = (uchar)HexDigitToL(map[9]);
      writeData.Port10 = (uchar)HexDigitToL(map[10]);
      writeData.Port11 = (uchar)HexDigitToL(map[11]);
      writeData.Port12 = (uchar)HexDigitToL(map[12]);
      writeData.Port13 = (uchar)HexDigitToL(map[13]);
      writeData.Port14 = (uchar)HexDigitToL(map[14]);
      writeData.Port15 = (uchar)HexDigitToL(map[15]);

      writeData.Checksum = ExpanderI2CChecksum( (uchar *)&writeData, sizeof(struct ExpanderPortPhyControlBlock) );

      printf("Sending Port Enable (Block 32) info to Expander: ");

      printf("%x", (int)writeData.Port1);
      printf("%x ", (int)writeData.Port0);
      printf("%x", (int)writeData.Port3);
      printf("%x ", (int)writeData.Port2);
      printf("%x", (int)writeData.Port5);
      printf("%x ", (int)writeData.Port4);
      printf("%x", (int)writeData.Port7);
      printf("%x ", (int)writeData.Port6);
      printf("%x", (int)writeData.Port9);
      printf("%x ", (int)writeData.Port8);
      printf("%x", (int)writeData.Port11);
      printf("%x ", (int)writeData.Port10);
      printf("%x", (int)writeData.Port13);
      printf("%x ", (int)writeData.Port12);
      printf("%x", (int)writeData.Port15);
      printf("%x ", (int)writeData.Port14);
      printf("\n");

      // Write the block to expander
      rc = WriteExpanderI2cDataBlock( (uchar *)&writeData, I2C_DATA_BUFFER_PHY_CONTROL_BLK_OFFSET,
                                       sizeof(struct ExpanderPortPhyControlBlock) );

      if( rc == DIAG_RC_NONE )
      {
         printf("Success.\n");
      }
      else
      {
         printf( "sendSwitchPortPhyControlToExpander: failed\n" );
      }
   }
   else
   {
       printf("Invalid argument.\n");
   }

   return rc;
}

int ReadExpanderInterrupt(void)
{
   char value = gpioIntRead(GPIO_DATA_IN_REG_EXPANDER_INT_BIT);
   printf("Expander interrupt: %d\n", value);
   return DIAG_RC_NONE;
}

