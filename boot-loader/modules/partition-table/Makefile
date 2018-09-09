# Storage modules.
ifeq ($(FIRMWARE),BIOS)
MBR := mbr
else ifeq ($(FIRMWARE),UEFI)
GPT := gpt
endif

PARTITION_TABLE_MODULES := $(MBR) $(GPT)
PARTITION_TABLE_MODULE_DIRS := $(patsubst %,$(PARTITION_TABLE)/%/,$(PARTITION_TABLE_MODULES))

include $(addsuffix Makefile,$(PARTITION_TABLE_MODULE_DIRS))

