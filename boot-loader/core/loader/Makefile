# Objects.
CORE_OBJS += $(LOADER)/loader.o $(LOADER)/symbols.o

# Module loaders.
ELF := elf
LOADER_MODULES_DIRS := $(patsubst %,$(LOADER)/%/,$(ELF))

include $(addsuffix Makefile,$(LOADER_MODULES_DIRS))

