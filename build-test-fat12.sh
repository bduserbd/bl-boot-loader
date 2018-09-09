#!/bin/sh

dd if=/dev/zero of=fat12.img bs=512 count=16384

parted fat12.img -s "mktable msdos"
parted fat12.img -s "mkpart p 2048s 10240s"

dd if=/dev/zero of=fat12-partition.img bs=512 count=8192
mkfs.vfat -F 12 fat12-partition.img
mmd -i fat12-partition.img ::/AAA
mmd -i fat12-partition.img ::/BBB
mmd -i fat12-partition.img ::/CCC
mmd -i fat12-partition.img ::/CCC/DDD
mmd -i fat12-partition.img ::/CCC/DDD/EEE
seq 1 10 > _test
mcopy -i fat12-partition.img _test ::/CCC/DDD/EEE/FFF
rm _test

dd if=fat12-partition.img of=fat12.img bs=512 seek=2048 count=8192 conv=notrunc

