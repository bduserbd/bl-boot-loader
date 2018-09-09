# Output directory.
OBJDIR := $(CURDIR)/objects-$(ARCH)/
_dummy := $(shell [ -d $(OBJDIR) ] || mkdir -p $(OBJDIR))

export OBJDIR

# Core sub-directories.
UTILS := utils
MEMORY := memory
LOADER := loader
VIDEO := video
BUS := bus
FS = fs
PARTITION_TABLE = partition-table
STORAGE := storage
KEYBOARD := keyboard
LINKER := linker
BL_SHELL := shell
GUI := gui

# Linker scripts.
LD_SCRIPT_TARGET := linker.ld
VERSION_SCRIPT_TARGET := linker.map
include $(LINKER)/Makefile

# Flags.
ifeq ($(ARCH),i386)
  CORE_CFLAGS += -m32 -march=i386 
  CORE_CPPFLAGS += -D BL_SIZEOF_PTR=4
endif

ifeq ($(FIRMWARE),BIOS)
  CORE_CFLAGS += -mregparm=3
  CORE_CPPFLAGS += -D FIRMWARE_BIOS
  CORE_CPPFLAGS += -D BL_EXECUTABLE
  CORE_LDFLAGS += -s
else ifeq ($(FIRMWARE),UEFI)
  CORE_CFLAGS += -mabi=ms
  CORE_CPPFLAGS += -D FIRMWARE_UEFI
  CORE_CPPFLAGS += -D BL_SHARED_LIBRARY
  CORE_CPPFLAGS += -D SECTION_ALIGNMENT=0x20
  CORE_LDFLAGS += -shared -Bsymbolic -znocombreloc
endif

CORE_CFLAGS += -Wall -O2
CORE_CFLAGS += -fno-strict-aliasing -fomit-frame-pointer
CORE_CFLAGS += -ffreestanding -mno-mmx -mno-sse -nostdlib
CORE_CFLAGS += -fno-stack-protector -mpreferred-stack-boundary=2
CORE_CFLAGS += -fvisibility=hidden
CORE_CFLAGS += -fshort-wchar
CORE_CFLAGS += -I$(DOT) -Iarch/$(ARCH)
CORE_CFLAGS += $(CORE_CPPFLAGS)

CORE_LDFLAGS += -T $(LD_SCRIPT_TARGET) --version-script=$(VERSION_SCRIPT_TARGET)
CORE_LDFLAGS += -m elf_$(ARCH)

CORE_OBJCOPYFLAGS += -O binary
CORE_OBJCOPYFLAGS += -R .comment -R .eh_frame
CORE_OBJCOPYFLAGS += -R .rel.eh_frame -R .note.GNU-stack

# Targets.
TARGET_LINK = bl.link
TARGET_BIN = bl.bin

##################
#    Building    #
##################

# Architecture.
include arch/$(ARCH)/Makefile

# Firmware.
include firmware/Makefile

# Core objects.
CORE_OBJS += init.o

# Use UEFI to perform memory allocations.
ifneq ($(FIRMWARE),UEFI)
  include $(MEMORY)/Makefile
endif

include $(UTILS)/Makefile
include $(BUS)/Makefile
include $(LOADER)/Makefile
include $(VIDEO)/Makefile
include $(PARTITION_TABLE)/Makefile
include $(FS)/Makefile
include $(STORAGE)/Makefile
include $(KEYBOARD)/Makefile
include $(BL_SHELL)/Makefile
include $(GUI)/Makefile

BUILD_OBJS += $(addprefix $(OBJDIR),$(notdir $(CORE_OBJS)))

# Image.
.PHONY := _all
_all: core-objects build-image

core-objects:
	$(V)$(MAKE) $(BUILD_OPT) OBJS="$(CORE_OBJS)" CFLAGS_OPT="$(CORE_CFLAGS)"

build-image:
	$(V)if [ "$(FIRMWARE)" = "BIOS" ]; then						\
		$(LD) $(CORE_LDFLAGS) -o $(TARGET_LINK) $(BUILD_OBJS);			\
		$(OBJCOPY) $(TARGET_LINK) $(TARGET_BIN)	$(CORE_OBJCOPYFLAGS);		\
		cp $(TARGET_BIN) $(DOTDOT)/$(BOOTLOADER_TARGET);			\
	elif [ "$(FIRMWARE)" = "UEFI" ]; then						\
		$(LD) $(CORE_LDFLAGS) -o $(TARGET_LINK) $(BUILD_OBJS);			\
		$(OBJCOPY) $(TARGET_LINK) $(TARGET_BIN)	$(CORE_OBJCOPYFLAGS);		\
		cp $(TARGET_BIN) $(DOTDOT)/$(BOOTLOADER_TARGET);			\
	fi;

