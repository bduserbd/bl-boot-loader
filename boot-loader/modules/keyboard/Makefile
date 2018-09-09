# Keyboard modules.
PS2_KBD := ps2-kbd
USB_KBD := usb-kbd

KEYBOARD_MODULES := $(USB_KBD) $(PS2_KBD)
KEYBOARD_MODULE_DIRS := $(patsubst %,$(KEYBOARD)/%/,$(KEYBOARD_MODULES))

include $(addsuffix Makefile,$(KEYBOARD_MODULE_DIRS))

