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
#include "command.h"
#include <linux/ctype.h>

#define IOC_SAS_ADDRESS_LENGTH 23
#define is_hex_digit(a) (((int)a>=(int)'0' && (int)a<=(int)'9') || \
                          ((int)a>=(int)'a' && (int)a<=(int)'f'))

static void DisplaySetIOCSasAddrHelp (void)
{
   printf ("Set IOC SAS Address\n");
   printf ("<address>\n");
   printf ("   address\n");
   printf ("      Should be of form 5x:xx:xx:xx:xx:xx:xx:xx\n");
   printf ("Don't forget to run saveenv to commit changes.\n");
   printf ("IOC will be programmed on next VSM boot.\n");

   printf ("\n");
} // DisplaySetIOCSasAddrHelp

int isSASAddressInMACFormat(char *addr)
{
   int rc = 0;

   // check for length
   int len = strlen(addr);
   if (len != IOC_SAS_ADDRESS_LENGTH)
   {
      rc = 1;
   }

   // check for format
   for(size_t n = 0; n < len && rc == 0; ++n)
   {
      if (n%3 == 2 && addr[n] != ':')
      {
         rc = 1;
         continue;
      }
      else if (n%3 == 0 || n%3 == 1)
      {
         int isHex = is_hex_digit(tolower(addr[n]));
         if (!isHex)
         {
            rc = 1;
         }
      }
   }

   // check for most significat nibble
   if (len > 0 && addr[0] != '5')
   {
      rc = 1;
   }

   return rc;
}

// *****************************************************************
// setiocsasaddr main routine
// *****************************************************************
static int DoSetIOCSASAddr (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]) 
{
   int   rc = 0;

   if (argc != 2)
   {
      printf("cmdtp->help length: %u\n", strlen(cmdtp->help) + 1);
      DisplaySetIOCSasAddrHelp();
   }
   else if (argc == 2)
   {
      if (isSASAddressInMACFormat( argv[1] ) == 0)
      {
          setenv("iocsasaddr",argv[1]);
          printf("Don't forget to run saveenv to commit changes.\n");
      }
      else
      {
          printf("Invalid address: %s\n", argv[1]);
          DisplaySetIOCSasAddrHelp();
          rc = 1;
      }
   }

   printf("RC: 0x%08X\n", rc);
   return rc;

} // DoSetIOCSASAddr

U_BOOT_CMD (
   setiocsasaddr, 2, 1, (void *)DoSetIOCSASAddr,
   "setiocsasaddr - set IOC SAS address\n",
// U-boot locks up or crashes when the help string is > 1020.
   "The setiocsasaddr command sets IOC SAS Address.\n"
);

