#
# This file is subject to the terms and conditions of the GNU General Public
# Licence.
# Copyright (C) 2018 Microsemi Corporation
#
#!/bin/sh
export CROSS_COMPILE_BH=arm-linux-gnueabi-
export CROSS_COMPILE=arm-linux-gnueabi-

ndb_flag=$1
if [ "${ndb_flag}"  != "ndb=1" ]
then 
   export DEBUG_BUILD=TRUE
fi

./build.pl -f spi -v 2013.01-2015_T1.0p18-galileo-gcc521sf-ddr4 -b armada_38x_customer1  -i spi -c -m4
make HOSTCC=arm-linux-gnueabihf-gcc env


