# Makefile for UEFI

UEFI_I386_OBJS := pe-ia32.o reloc.o
UEFI_I386_OBJS += boot.o relocate-ia32.o

CORE_OBJS += $(addprefix arch/$(ARCH)/uefi/,$(UEFI_I386_OBJS))

