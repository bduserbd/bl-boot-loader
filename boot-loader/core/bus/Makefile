# Buses.
PCI-BUS := pci
USB-BUS := usb

BUSES := $(PCI-BUS) $(USB-BUS)
BUSES_DIRS := $(patsubst %,$(BUS)/%/,$(BUSES))

include $(addsuffix Makefile,$(BUSES_DIRS))

