#!/bin/sh

# Do something like this:
dd if=/dev/zero of=ntfs.img bs=512 count=131072

parted ntfs.img -s "mktable msdos"
parted ntfs.img -s "mkpart p 2048s 67584s"

# dd if=/dev/zero of=ntfs-partition.img bs=... count=...
# sudo losetup /dev/loop0 ntfs-partition.img
# sudo mkfs.ntfs /dev/loop0
# sudo mount /dev/loop0 /mnt/ntfs -t ntfs
#
# Now you can create/delete files on the mounted partition.

dd if=ntfs-partition.img of=ntfs.img bs=512 seek=2048 count=65536 conv=notrunc

# Finally you should unmount the partition:
# sudo umount /mnt/ntfs
# sudo losetup -d /dev/loop0

