qemu-system-i386					\
	-device pci-ohci,id=ohci			\
	-device usb-hub,bus=ohci.0,port=1		\
	-device usb-kbd,bus=ohci.0,port=2		\
	-L ~/OVMF-IA32-r15214/				\
	-bios OVMF.fd					\
	-drive file=disk.img,format=raw,if=ide		\
	-monitor stdio

#	-device usb-ehci,id=ehci			\
#	-device usb-kbd,bus=ehci.0,port=1		\

