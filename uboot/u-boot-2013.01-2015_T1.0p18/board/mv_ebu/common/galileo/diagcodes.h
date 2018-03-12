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
#ifndef DIAG_CODES_H
#define DIAG_CODES_H

// OK for all diags test commands
#define DIAG_RC_NONE                         0x00000000
                                             
// General error codes for all the commands
#define DIAG_RC_ILLEGAL_ARGUMENT             0x00000001
#define DIAG_RC_BAD_ARGUMENT_COUNT           0x00000002
                                             
// Generic flash test errors                 
#define DIAG_RC_FLASH_ERASE_ERROR            0x00010001  // internal only
#define DIAG_RC_FLASH_WRITE_ERROR            0x00010002  // internal only
#define DIAG_RC_FLASH_COMPARE_ERROR          0x00010003  // internal only
#define DIAG_RC_FLASH_MALLOC_ERROR           0x00010004
// ROM 1 flash test errors                   
#define DIAG_RC_FLASH_ROM1_MASK              0x00000100  // internal only
#define DIAG_RC_FLASH_ERASE_ERROR_ROM1       0x00010101
#define DIAG_RC_FLASH_WRITE_ERROR_ROM1       0x00010102
#define DIAG_RC_FLASH_COMPARE_ERROR_ROM1     0x00010103
// ROM 2 flash test errors                   
#define DIAG_RC_FLASH_ROM2_MASK              0x00000200  // internal only
#define DIAG_RC_FLASH_ERASE_ERROR_ROM2       0x00010201
#define DIAG_RC_FLASH_WRITE_ERROR_ROM2       0x00010202
#define DIAG_RC_FLASH_COMPARE_ERROR_ROM2     0x00010203
                                             
// IC Revision errorts                       
#define DIAG_RC_IC_REV_INVALID_IMPLEMENTOR   0x00030001
#define DIAG_RC_IC_REV_INVALID_VARIANT       0x00030002
#define DIAG_RC_IC_REV_INVALID_PART_NUMBER   0x00030003
#define DIAG_RC_IC_REV_INVALID_REVISION      0x00030004
                                             
// NVRAM test errors                         
#define DIAG_RC_NVRAM_READ_ERROR             0x00040001
#define DIAG_RC_NVRAM_MALLOC_ERROR           0x00040002

// EMIC test errors 
#define DIAG_RC_EMIC_ACCESS_ERROR          0x00050001 // CANMIC inaccessible or I2C bus error
#define DIAG_RC_EMIC_INIT_ERROR            0x00050002 // CANMIC init(sequence) error
#define DIAG_RC_EMIC_OP_READ_ERROR         0x00050003
#define DIAG_RC_EMIC_OP_WRITE_ERROR        0x00050004
#define DIAG_RC_EMIC_OP_DIRTY_ERROR        0x00050005
#define DIAG_RC_EMIC_OP_START_INIT_ERROR   0x00050006
#define DIAG_RC_EMIC_OP_STOP_INIT_ERROR    0x00050007
#define DIAG_RC_EMIC_VSM_RESET_ERROR       0x00050008 // VSM reset(via canmic) error
#define DIAG_RC_EMIC_OP_UNINIT_ERROR       0x00050009 // CANMIC uninit error
                                             
// CPLD test errors 
#define DIAG_RC_CPLD_I2C_READ                0x00060001
#define DIAG_RC_CPLD_NOT_SUPPORTED           0x00060002

// Identify board test errors                
#define DIAG_RC_ID_BOARD_NVRAM_READ          0x00070002
#define DIAG_RC_ID_BOARD_MALLOC_ERROR        0x00070003
#define DIAG_RC_ID_BOARD_TAG_NOT_FOUND       0x00070004
                                             
/* Expander I2C error definitions */
#define DIAG_RC_EXP_I2C_READ_ERROR           0x00080001
#define DIAG_RC_EXP_I2C_WRITE_ERROR          0x00080002
#define DIAG_RC_EXP_I2C_DATA_ERROR           0x00080004 // data integrity check failed

#define DIAG_RC_EMIC_INTTERUPT_NOT_SET     0x00090001
#define DIAG_RC_EXP_INTERRUPT_NOT_SET        0x00090002

// ethernet test errors                      
#define DIAG_RC_BAD_IP_ADDRESS               0x000A0001
#define DIAG_RC_HOST_NOT_ALIVE               0x000A0002
                                                                                        
// Update wellness log errors                
#define DIAG_RC_UP_WELL_ILLEGAL_VALUE        0x000B0001
#define DIAG_RC_UP_WELL_MALLOC_ERROR         0x000B0002
#define DIAG_RC_UP_WELL_NVRAM_READ           0x000B0003
#define DIAG_RC_UP_WELL_TAG_NOT_FOUND        0x000B0004
#define DIAG_RC_UP_WELL_ICT_NOT_FOUND        0x000B0005
#define DIAG_RC_UP_WELL_NVRAM_WRITE          0x000B0006
                                             
// NVRAM log test errors                     
#define DIAG_RC_NVRAM_LOG_TOO_SMALL          0x000C0001
#define DIAG_RC_NVRAM_LOG_MALLOC_ERROR       0x000C0002
#define DIAG_RC_NVRAM_LOG_NVRAM_READ         0x000C0003
#define DIAG_RC_NVRAM_LOG_NVRAM_WRITE        0x000C0004
#define DIAG_RC_NVRAM_LOG_NVRAM_VERIFY       0x000C0005
                                            
// Diags Initialization
#define DIAG_RC_INIT_DIAGS_MODE_FAILED       0x000D0001

// Cleanup
#define DIAG_RC_CLEANUP_FAILED               0x000E0001
 
// Memory test errors                        
#define DIAG_RC_MEM_DMA_ERROR                0x01000001
#define DIAG_RC_MEM_VERIFY_ERROR             0x01000002
                      
// PCI test errors 
#define DIAG_RC_PCIE_BOARD_INFO_ERROR        0x02000001
#define DIAG_RC_PCIE_GEN_VERIFY_ERROR        0x02000002
#define DIAG_RC_PCIE_ID_COMPARE_ERROR        0x02000003
#define DIAG_RC_PCIE_SCAN_ERROR              0x02000004
#define DIAG_RC_PCIE_NO_LINK_ERROR           0x02000005
#define DIAG_RC_PCIE_WIDTH_VERIFY_ERROR      0x02000006
                       
#endif // DIAG_CODES_H
