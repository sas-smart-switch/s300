#
# This file is subject to the terms and conditions of the GNU General Public
# Licence.
# Copyright (C) 2018 Microsemi Corporation
#


#!/usr/bin/python
import os
originalUbootFile = raw_input("Please input u-boot filename: ")
originalFileSize  = os.path.getsize(originalUbootFile)
print 'Original u-boot file size: '+ str(originalFileSize)
fourMiBinByte = 1048576 * 4;
print '4MiB in bytes: ' + str(fourMiBinByte)
sizeToPad = fourMiBinByte - originalFileSize
print 'Size to pad : '  + str(sizeToPad)
convert = r'tr "\000" "\377" '
prepaddedFourM = 'uboot4M.bin'
removeStaleFile = 'rm -frv ' + prepaddedFourM;
paddingCMDFF = 'dd if=/dev/zero bs=1 count='+str(fourMiBinByte) + '| ' + convert  + '  > '  + prepaddedFourM
print 'Remove stale file and generate '+ prepaddedFourM 
os.system(removeStaleFile);
print 'Generating '+ prepaddedFourM + ' in 4M size with oxFF'
os.system(paddingCMDFF);

paddingCMD = 'dd if='+ originalUbootFile + ' of='+ prepaddedFourM + ' conv=notrunc'
print 'Startig to pad u-boot to 4M size, please wait ...' 
os.system(paddingCMD);
print 'Done: ' + originalUbootFile  +' has been padded to 4MiB, byte number: ' + str(fourMiBinByte)
print 'Use this final 4M file: ' + prepaddedFourM
