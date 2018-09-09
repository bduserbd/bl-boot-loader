# USB modules.
UHCI := uhci
OHCI := ohci
EHCI := ehci
XHCI := xhci

USB_MODULES := $(UHCI) $(OHCI) $(EHCI) $(XHCI)
USB_MODULES_DIRS := $(patsubst %,$(USB)/%/,$(USB_MODULES))

include $(addsuffix Makefile,$(USB_MODULES_DIRS))

