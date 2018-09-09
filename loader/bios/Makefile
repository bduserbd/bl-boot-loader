# Makefile for BIOS bootloader (MBR)

# Targets.
loader := loader.bin
bl-loader := bl-loader.bin

# Build flags.
REALMODE_CFLAGS += -m32 -march=i386
REALMODE_CFLAGS += -Wall -Os -mregparm=3
REALMODE_CFLAGS += -Wstrict-prototypes -mpreferred-stack-boundary=2
REALMODE_CLFAGS += -fno-strict-aliasing -fomit-frame-pointer -fno-pic
REALMODE_CLFAGS += -mno-mmx -mno-sse -ffreestanding -fno-stack-protector

OBJCOPYFLAGS = -O binary -R .eh_frame -R .comment

# Building.
PHONY := _all
_all: build clean

build: $(loader) $(bl-loader)
	$(V)cp $(loader) $(DOTDOT)/$(LOADER_TARGET)
	$(V)cp $(bl-loader) $(DOTDOT)/$(BL_LOADER_TARGET)

%.o: %.S
	$(V)echo '  AS ' $(shell basename $(CURDIR))/$@
	$(V)$(CC) $(REALMODE_CFLAGS) -c $^

%.elf: %.o
	$(V)echo '  LD ' $(shell basename $(CURDIR))/$@
	$(V)$(LD) -Tlinker-$(basename $@).ld -o $@ $<

%.bin: %.elf
	$(V)echo '  OBJCOPY ' $(shell basename $(CURDIR))/$@
	$(V)$(OBJCOPY) $< $@ $(OBJCOPYFLAGS)

clean:
	$(V)rm *.o *.bin *.elf

