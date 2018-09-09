# Verbosity
MAKEFLAGS += -I$(CURDIR) --no-print-directory

V = @
ifeq "$(origin VERBOSE)" "command line"
ifneq ($(VERBOSE), 0)
  undefine V
endif
endif

export V

# Common tools.
PYTHON = python

export PYTHON

# Used flags.
BUILD_CFLAGS := -I$(CURDIR)

export BUILD_CFLAGS

# Directories.
DOT = .
DOTDOT = ..

export DOT DOTDOT

# Main target directories & objects.
LOADER := loader
BOOTLOADER := boot-loader
TOOLS := tools

BOOTLOADER_TARGET := boot-loader.img
FINAL_TARGET := disk.img

export BOOTLOADER_TARGET

# Determine build architecture.
SYSTEM_ARCH := $(shell uname -p)
ARCH ?= $(SYSTEM_ARCH)

export SYSTEM_ARCH ARCH

# Tools.
ifeq ($(ARCH),i386)
  CROSS_COMPILE := $(patsubst %gcc,%,$(shell command -v i686-linux-gnu-gcc      \
                        2> /dev/null 2>&1 || command -v x86_64-linux-gnu-gcc    \
                        2> /dev/null 2>&1))
  ifndef CROSS_COMPILE
    $(error Unproper toolchain)
  endif
endif

CC := $(CROSS_COMPILE)gcc
AS := $(CROSS_COMPILE)as
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy

export CC AS LD OBJCOPY

# Firmware type.
ifeq ($(FIRMWARE), BIOS)
  include Makefile.bios
  TARGETS := loader-target boot-loader-target
else ifeq ($(FIRMWARE), UEFI)
  include Makefile.uefi
  TARGETS := boot-loader-target
else
  $(error Unsupported firmware type (choose BIOS/UEFI))
endif

export FIRMWARE

# Build targets
.PHONY := _all
_all: $(TARGETS) image-target

loader-target:
	$(V)echo '[[ Building Loader : ]]'
	$(V)cd $(LOADER) && $(MAKE)
	$(V)echo '[[ Finished building Loader ]]'

boot-loader-target:
	$(V)echo '[[ Building Boot Loader : ]]'
	$(V)cd $(BOOTLOADER) && $(MAKE)
	$(V)echo '[[ Finished building Boot Loader ]]'

image-target:
	$(V)echo '[[ Building image : ]]'
	$(call build_image,$(FINAL_TARGET))
	$(V)echo '[[ Finished building image ]]'

