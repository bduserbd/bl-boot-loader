# Video modules.
ifeq ($(FIRMWARE),BIOS)
VBE := vbe
else ifeq ($(FIRMWARE),UEFI)
UEFI := uefi
endif

VIDEO_MODULES := $(VBE) $(UEFI)
VIDEO_MODULES_DIRS := $(patsubst %,$(VIDEO)/%/,$(VIDEO_MODULES))

include $(addsuffix Makefile,$(VIDEO_MODULES_DIRS))

