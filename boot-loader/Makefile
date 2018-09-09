# Build tools.
BUILD_SCRIPT := $(CURDIR)/Makefile.build
BUILD_OPT := -f $(BUILD_SCRIPT)
BUILD_CFLAGS += -I$(CURDIR)

# Current directory.
BLDIR = $(CURDIR)

export BLDIR BUILD_OPT

# Target directories.
core-dir = core/
modules-dir = modules/

# Define build targets.
.PHONY := _all
_all: core-target modules-target

core-target:
	$(V)$(MAKE) -C $(core-dir)

modules-target:
	$(V)$(MAKE) -C $(modules-dir)

