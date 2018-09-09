# Build various linker scripts.

ifeq ($(FIRMWARE),BIOS)
  ifeq ($(ARCH),i386)
    LD_TARGETS := bios-i386.ld.S bios-i386.map
  endif
else ifeq ($(FIRMWARE),UEFI)
  ifeq ($(ARCH),i386)
    LD_TARGETS := uefi-ia32.ld.S uefi-ia32.map
  endif
endif

# Targets.
.PHONY := _all
_all: $(LD_TARGETS)

%.ld.S:
	$(V)$(CC) $(CORE_CPPFLAGS) -E -x c $(LINKER)/$@ | grep -v '^#' > $(LD_SCRIPT_TARGET)

%.map:
	$(V)cp $(LINKER)/$@ $(VERSION_SCRIPT_TARGET)

