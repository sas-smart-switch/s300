#!/bin/sh

export CROSS_COMPILE=output/host/usr/bin/aarch64-linux-gnu-
for f in `ls output/images/rootfs.ext*`; do
	./board/marvell/scripts/mkimage -O linux -T ramdisk -A arm64 -C none -n "Ramdisk" -d $f $(dirname $f)/u_$(basename $f)
done
echo "Done."
