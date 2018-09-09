# Makefile for firmware.

ifeq ($(FIRMWARE), BIOS)
  FIRMWARE_TARGET := bios/
endif

# Targets
.PHONY := _all
_all: firmware

firmware:
	$(V)$(MAKE) -C $(FIRMWARE_TARGET)

