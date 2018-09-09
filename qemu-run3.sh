sudo qemu-system-i386						\
	-device pci-ohci,id=ohci				\
	-device usb-kbd,bus=ohci.0,port=1			\
	-drive if=none,id=stick,file=/dev/sdb			\
	-device usb-storage,drive=stick,bus=ohci.0,port=2	\
	-drive file=disk.img,format=raw,if=ide			\
	-monitor stdio

#	-device usb-ehci,id=ehci			\
#	-device usb-kbd,bus=ehci.0,port=1		\

