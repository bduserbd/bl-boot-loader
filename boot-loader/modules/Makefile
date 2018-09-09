# Output directory.
OBJDIR := $(CURDIR)/objects/
_dummy := $(shell [ -d $(OBJDIR) ] || mkdir -p $(OBJDIR))

export OBJDIR

# Modules.
PARTITION_TABLE := partition-table
STORAGE := storage
FS := fs
USB := usb
KEYBOARD := keyboard
VIDEO := video

MODULE_DIRS += $(PARTITION_TABLE)/
MODULE_DIRS += $(STORAGE)/
MODULE_DIRS += $(FS)/
MODULE_DIRS += $(USB)/
MODULE_DIRS += $(KEYBOARD)/
MODULE_DIRS += $(VIDEO)/

# Flags.
ifeq ($(ARCH),i386)
  MODULES_CFLAGS += -m32 -march=i386
endif

ifeq ($(FIRMWARE),BIOS)
  MODULES_CFLAGS += -mregparm=3
else ifeq ($(FIRMWARE),UEFI)
  MODULES_CFLAGS += -mabi=ms
endif

MODULES_CFLAGS += -Wall -O2
MODULES_CFLAGS += -fno-strict-aliasing -fomit-frame-pointer -fno-pic
MODULES_CFLAGS += -ffreestanding -mno-mmx -mno-sse
MODULES_CFLAGS += -fno-stack-protector -mpreferred-stack-boundary=2
MODULES_CFLAGS += -fshort-wchar

MODULES_OBJCOPYFLAGS += -R .comment -R .eh_frame
MODULES_OBJCOPYFLAGS += -R .rel.eh_frame -R .note.GNU-stack

# Targets.
include $(addsuffix Makefile,$(MODULE_DIRS))

.PHONY := _all
_all: build-modules

define strip_unneeded_sections
	$(OBJCOPY) $(OBJDIR)$(notdir $(1)) $(MODULES_OBJCOPYFLAGS);
endef

build-modules:
	$(V)$(MAKE) $(BUILD_OPT) OBJS="$(MODULE_OBJS)" CFLAGS_OPT="$(MODULES_CFLAGS)"
	$(V)$(foreach m,$(MODULE_OBJS),$(call strip_unneeded_sections,$(m)))

