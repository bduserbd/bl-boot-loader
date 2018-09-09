# Real mode related objects.
REALMODE_OBJS += main.o sys-main.o die.o
REALMODE_OBJS += cpu-modes.o cpu.o
REALMODE_OBJS += bios.o bios-interrupt.o
REALMODE_OBJS += a20.o

CORE_OBJS += $(addprefix arch/$(ARCH)/real-mode/,$(REALMODE_OBJS))

