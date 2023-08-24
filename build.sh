#!/bin/bash

export ARCH=arm64
export CROSS_COMPILE="ccache /usr/bin/aarch64-linux-gnu-"

echo "-- building --"


make -j 8 Image.gz dtbs modules

echo "-- installing --"

#sudo make modules_install INSTALL_MOD_PATH=/media/mimoja/rootfs

sudo cp arch/arm64/boot/dts/freescale/imx8mn-var-som-symphony.dtb /media/mimoja/rootfs/boot/imx8mn-var-som-symphony.dtb
sudo cp arch/arm64/boot/Image.gz /media/mimoja/rootfs/boot/Image.gz
echo "-- installed as default kernel --"

echo "-- syncing --"
sync

echo "-- to boot stock kernel --"
echo setenv fdt_file stock.dtb
echo setenv image stock.gz
echo boot

params=$(grep -e "met_mode = ." drivers/gpu/drm/panel/panel-sitronix-st7701.c -A 25 | grep "};" -B 25 | sed "s/[\t .,;]*//g" | sed "s/.*[\{\}]//g")
echo "60 * ((480 + 38 + 12 + 14) * (480 + 2 + 48 + 12)) / 1000 = X"

exit
