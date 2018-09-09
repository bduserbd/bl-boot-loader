sudo qemu-system-i386						\
	-enable-kvm						\
	-device nec-usb-xhci,id=xhci				\
	-drive file=disk.img,format=raw,if=ide			\
	-monitor stdio

#	-device usb-ehci,id=ehci			\
#	-device usb-kbd,bus=ehci.0,port=1		\
#	-device usb-hub,bus=xhci.0,port=1			\
#	-device usb-kbd,bus=xhci.0,port=4			\
#	-device usb-host,vendorid=0x0781,productid=0x5567	\
#	-device usb-hub,bus=xhci.0,port=2			\
#	-device usb-kbd,bus=xhci.0,port=3			\

