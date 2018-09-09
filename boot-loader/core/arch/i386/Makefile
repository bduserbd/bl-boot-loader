# Makefile for i386.

ifeq ($(FIRMWARE),BIOS)

I386_DIRS := real-mode pit mtrr
include $(patsubst %,arch/$(ARCH)/%/Makefile,$(I386_DIRS))

else ifeq ($(FIRMWARE),UEFI)

include arch/$(ARCH)/uefi/Makefile

endif

