# File system modules.
FAT := #fat
EXT := ext
NTFS := ntfs

FS_MODULES := $(FAT) $(EXT) $(NTFS)
FS_MODULES_DIRS := $(patsubst %,$(FS)/%/,$(FS_MODULES))

include $(addsuffix Makefile,$(FS_MODULES_DIRS))

