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
 * @file expanderi2c.c 
 * @brief Diagnostic function to test VSM to Expander I2C 
 *        interface
 */

#include "common.h"
#include <i2c.h>
#include "expanderi2c.h"

// *****************************************************************
// Read Expander I2C data block
// *****************************************************************
static int ReadExpanderI2cDataBlock( uchar *pData, uchar offset, int length )
{
   int rc = DIAG_RC_NONE;

   i2c_set_bus_num( EXPANDER_I2C_BUS );

   if( i2c_read( EXPANDER_I2C_ADDRESS, offset, 1, pData, length ) )
   {
      printf( "ReadExpanderI2cDataBlock: Failed\n" );
      rc = DIAG_RC_EXP_I2C_READ_ERROR;
   }

   return rc;
}

// *****************************************************************
// Write Expander I2C data block
// *****************************************************************
int WriteExpanderI2cDataBlock( uchar *pData, uchar offset, int length )
{
   int rc = 0;

   i2c_set_bus_num( EXPANDER_I2C_BUS );

   rc = i2c_write( EXPANDER_I2C_ADDRESS, offset, 1, pData, length );
   if(rc)
   {
      printf( "WriteExpanderI2cDataBlock: Failed. Code:%d\n", rc);
      rc = DIAG_RC_EXP_I2C_WRITE_ERROR;
   }
   else
   {
      rc = DIAG_RC_NONE;
   }

   return rc;
}

// *****************************************************************
// Calculate Checksum for a given data block
//    checksum is two's complement of modulo 256 sum
// *****************************************************************
uchar ExpanderI2CChecksum( const uchar* pData, uint length )
{
   uint i = 0;
   uchar checksum = 0;
   
   for( i = 0; i < length; i++ )
   {
      checksum += pData[i];
   }
   checksum = ~checksum + 1;

   return checksum;
}

// *****************************************************************
// Test Expander I2C
// *****************************************************************
int TestExpanderI2c( )
{
   int rc = DIAG_RC_NONE;
   struct ExpanderDiagnosticBlock readData = {0};
   struct ExpanderDiagnosticBlock writeData = {0};

   // Fill the diagnostic block to be written
   writeData.BlockNum = EXPANDER_DIAGNOSTIC_BLK;
   strncpy( writeData.Signature, "VSM.", EXP_I2C_DIAG_BLK_SIGNATURE_LENGTH );
   writeData.Checksum = ExpanderI2CChecksum( (uchar *)&writeData, sizeof(struct ExpanderDiagnosticBlock) );

   // I2C Write-Read-Verify the diagnostic block
   rc = WriteExpanderI2cDataBlock( (uchar *)&writeData, I2C_DATA_BUFFER_DIAGNOSTIC_BLK_OFFSET,
                                    sizeof(struct ExpanderDiagnosticBlock) );

   if( rc == DIAG_RC_NONE )
   {
      rc = ReadExpanderI2cDataBlock( (uchar *)&readData, I2C_DATA_BUFFER_DIAGNOSTIC_BLK_OFFSET,
                                     sizeof(struct ExpanderDiagnosticBlock) );
      if( rc == DIAG_RC_NONE )
      {
         if( memcmp(&readData, &writeData, sizeof(struct ExpanderDiagnosticBlock) ) )
         {
            printf( "TestExpanderI2c: Data Check Failed\n" );
            rc = DIAG_RC_EXP_I2C_DATA_ERROR;
         }
      }
   }
   
   if( rc == DIAG_RC_NONE )
   {
      printf( "Expander I2C test successful\n" );
   }
   else
   {
      printf( "Expander I2C test unsuccessful: %x \n", rc );
   }

   return rc;
}

// *****************************************************************
// Send Switch Bay Number to Expander
// *****************************************************************
int sendSwitchBayNumToExpander( uchar bayNum )
{
   int rc = DIAG_RC_NONE;
   struct ExpanderBayNumberBlock writeData = {0};

   // Fill the bay number block to be written
   writeData.BlockNum = EXPANDER_BAY_NUMBER_BLK;
   writeData.BayNum = bayNum;
   writeData.Checksum = ExpanderI2CChecksum( (uchar *)&writeData, sizeof(struct ExpanderBayNumberBlock) );

   // Write the block to expander
   rc = WriteExpanderI2cDataBlock( (uchar *)&writeData, I2C_DATA_BUFFER_BAY_NUMBER_BLK_OFFSET,
                                    sizeof(struct ExpanderBayNumberBlock) );

   if( rc != DIAG_RC_NONE )
   {
      printf( "sendSwitchBayNumToExpander: failed\n" );
   }

   return (DIAG_RC_NONE == rc) ? TRUE : FALSE;
}

// *****************************************************************
// Send Switch Port Phy Control to Expander
// *****************************************************************
int setupAllExpanderPhys( const MV_BOOL isManufactureMode )
{
   int rc = DIAG_RC_NONE;
   struct ExpanderPortPhyControlBlock writeData = {0};

   // Fill the phy control block to be written
   writeData.BlockNum = EXPANDER_PORT_PHY_CONTROL_BLK;

   // Disable Cassini external ports in normal mode. Enable all ports in manufacturing mode
   uchar externalPortModeValue = 0x0;
   if (isManufactureMode)
   {
      externalPortModeValue = 0xf;
      printf("Sending Port Enable info to Expander: ");
   }
   else
   {
      printf("Sending external port disable info to Expander: ");
   }
  
   writeData.Port0 = 0xf;
   writeData.Port1 = 0xf;
   writeData.Port2 = 0xf;
   writeData.Port3 = 0xf;
   writeData.Port4 = 0xf;
   writeData.Port5 = 0xf;
   writeData.Port6 = 0xf;
   writeData.Port7 = 0xf;
   writeData.Port8 = externalPortModeValue;
   writeData.Port9 = externalPortModeValue;
   writeData.Port10 = externalPortModeValue;
   writeData.Port11 = externalPortModeValue;
   writeData.Port12 = externalPortModeValue;
   writeData.Port13 = externalPortModeValue;
   writeData.Port14 = externalPortModeValue;
   writeData.Port15 = externalPortModeValue;
   writeData.Checksum = ExpanderI2CChecksum( (uchar *)&writeData, sizeof(struct ExpanderPortPhyControlBlock) );

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

   if( rc != DIAG_RC_NONE )
   {
      printf( "sendSwitchPortPhyControlToExpander: failed\n" );
   }

   return (DIAG_RC_NONE == rc) ? TRUE : FALSE;
}

