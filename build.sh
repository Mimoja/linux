#!/bin/bash

export ARCH=arm64
export CROSS_COMPILE="ccache /usr/bin/aarch64-linux-gnu-"

echo "-- building --"

make -j 8 Image.gz dtbs modules

echo "-- installing --"

#sudo make modules_install INSTALL_MOD_PATH=/media/mimoja/rootfs

sudo cp arch/arm64/boot/dts/freescale/imx8mn-var-som-symphony.dtb /media/mimoja/rootfs/boot/imx8mn-var-som-symphony.dtb
sudo cp arch/arm64/boot/Image.gz /media/mimoja/rootfs/boot/Image.gz

echo "-- syncing --"
sync

echo "-- boot stock kernel as"
echo setenv fdt_file stock.dtb
echo setenv image stock.gz
echo boot