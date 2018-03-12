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
 * @file expanderi2c.h
 * @brief Header file for expanderi2c.c  
 *  
 */

#ifndef EXPANDER_I2C_H
#define EXPANDER_I2C_H

#include "diagcodes.h"
#include "mvTypes.h"

/* I2C bus interface definitions */
#define EXPANDER_I2C_BUS                     1
#define EXPANDER_I2C_ADDRESS                 0x41
#define EXPANDER_I2C_SLAVE_DATA_BUFFER_SIZE  256

/* Expander I2C data block numbers */
#define EXPANDER_DIAGNOSTIC_BLK              0x01  // diagnostic block
#define EXPANDER_BAY_NUMBER_BLK              0x02  // bay number block
#define EXPANDER_PORT_PHY_CONTROL_BLK        0x03  // port phy control block

#define EXP_I2C_DIAG_BLK_SIGNATURE_LENGTH    4

#pragma pack(push) // save current structure alignment

#pragma pack(1)    // set structure alignment for 1 byte
///////////////////////////////////////////////////////////////////////
/// @struct  ExpanderDiagnosticBlock
/// @brief   This structure defines the diagnostic data block
///          sent by VSM; Diagnostic is a no-op data block that VSM
///          uses to write-read-verify the I2C interface with expander
///////////////////////////////////////////////////////////////////////
struct ExpanderDiagnosticBlock
{
   uchar BlockNum;
   char  Signature[EXP_I2C_DIAG_BLK_SIGNATURE_LENGTH];
   uchar Checksum;
};

///////////////////////////////////////////////////////////////////////
/// @struct  ExpanderBayNumberBlock
/// @brief   This structure defines the bay number data block
///          sent by VSM
///////////////////////////////////////////////////////////////////////
struct ExpanderBayNumberBlock
{
   uchar BlockNum;
   uchar BayNum;
   uchar Align; // padding to even length align the data block
   uchar Checksum;
};

///////////////////////////////////////////////////////////////////////
/// @struct  ExpanderPortPhyControlBlock
/// @brief   This structure defines the port phy control data block
///          sent by VSM
///////////////////////////////////////////////////////////////////////
struct ExpanderPortPhyControlBlock
{
   uchar BlockNum;

   uchar Port14:4; // phy control bits for port 14
   uchar Port15:4; // phy control bits for port 15
   uchar Port12:4; // phy control bits for port 12
   uchar Port13:4; // phy control bits for port 13
   uchar Port10:4; // phy control bits for port 10
   uchar Port11:4; // phy control bits for port 11
   uchar Port8:4;  // phy control bits for port 8
   uchar Port9:4;  // phy control bits for port 9
   uchar Port6:4;  // phy control bits for port 6
   uchar Port7:4;  // phy control bits for port 7
   uchar Port4:4;  // phy control bits for port 4
   uchar Port5:4;  // phy control bits for port 5
   uchar Port2:4;  // phy control bits for port 2
   uchar Port3:4;  // phy control bits for port 3
   uchar Port0:4;  // phy control bits for port 0
   uchar Port1:4;  // phy control bits for port 1

   uchar Checksum;
};

///////////////////////////////////////////////////////////////////////
/// @struct  ExpanderI2cSlaveDataBuffer
/// @brief   This structure defines the Expander I2C slave data buffer.
/// @details The individual block format is inherited from the original
///          ISMIC data block model. The structure here mimics a I2C
///          NVRAM or EEPROM type of data format with all data blocks
///          defined at fixed offsets. Regardless of what interface
///          (ISMIC or CANMIC) is used upstream 12G VSM(and may be 
///          later) will always use this data buffer model for
///          passing info downstream to the expander(s)
///////////////////////////////////////////////////////////////////////
struct ExpanderI2cSlaveDataBuffer
{
   struct ExpanderDiagnosticBlock      Diagnostic;
   struct ExpanderBayNumberBlock       BayNumber;
   struct ExpanderPortPhyControlBlock  PortPhyControl;
   uchar  Reserved[EXPANDER_I2C_SLAVE_DATA_BUFFER_SIZE - 
      sizeof(struct ExpanderDiagnosticBlock) - 
      sizeof(struct ExpanderBayNumberBlock) - 
      sizeof(struct ExpanderPortPhyControlBlock)];
};
#pragma pack(pop) // restore saved structure alignment

/* Expander I2C data buffer block offset definitions */
#define I2C_DATA_BUFFER_DIAGNOSTIC_BLK_OFFSET   (offsetof(struct ExpanderI2cSlaveDataBuffer, Diagnostic))
#define I2C_DATA_BUFFER_BAY_NUMBER_BLK_OFFSET   (offsetof(struct ExpanderI2cSlaveDataBuffer, BayNumber))
#define I2C_DATA_BUFFER_PHY_CONTROL_BLK_OFFSET  (offsetof(struct ExpanderI2cSlaveDataBuffer, PortPhyControl))

/* Prototypes of Expander I2C test external facing functions */
int TestExpanderI2c( void );
int sendSwitchBayNumToExpander( uchar bayNum );
uchar ExpanderI2CChecksum( const uchar* pData, uint length );
int WriteExpanderI2cDataBlock( uchar *pData, uchar offset, int length );
int setupAllExpanderPhys( const MV_BOOL isManufactureMode );

#endif // EXPANDER_I2C_H
