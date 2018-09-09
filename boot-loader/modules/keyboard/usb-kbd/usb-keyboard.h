#ifndef BL_USB_KEYBOARD_H
#define BL_USB_KEYBOARD_H

/* Modifier keys in report. */
enum {
        BL_USB_MODIFIER_KEY_LEFT_CTRL   = (1 << 0),
        BL_USB_MODIFIER_KEY_LEFT_SHIFT  = (1 << 1),
        BL_USB_MODIFIER_KEY_LEFT_ALT    = (1 << 2),
        BL_USB_MODIFIER_KEY_LEFT_GUI    = (1 << 3),
        BL_USB_MODIFIER_KEY_RIGHT_CTRL  = (1 << 4),
        BL_USB_MODIFIER_KEY_RIGHT_SHIFT = (1 << 5),
        BL_USB_MODIFIER_KEY_RIGHT_ALT   = (1 << 6),
        BL_USB_MODIFIER_KEY_RIGHT_GUI   = (1 << 7),
};

#endif

