# Storage modules.
PATA := pata
AHCI := ahci
USB_SCSI := usb-scsi

STORAGE_MODULES := $(PATA) $(AHCI) $(USB_SCSI)
STORAGE_MODULE_DIRS := $(patsubst %,$(STORAGE)/%/,$(STORAGE_MODULES))

include $(addsuffix Makefile,$(STORAGE_MODULE_DIRS))

