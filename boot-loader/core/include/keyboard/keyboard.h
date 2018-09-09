#ifndef BL_KEYBOARD_H
#define BL_KEYBOARD_H

#include "include/bl-types.h"
#include "include/error.h"

typedef enum {
	BL_KEYBOARD_PS2,
	BL_KEYBOARD_USB,
} bl_keyboard_t;

struct bl_keyboard;

struct bl_keyboard_controller {
	bl_keyboard_t type;

	bl_status_t (*initialize)(struct bl_keyboard *);
	int (*getchar)(struct bl_keyboard *);

	struct bl_keyboard_controller *next;
};

struct bl_keyboard {
	bl_keyboard_t type;

	void *data;
	struct bl_keyboard_controller *controller;

	struct bl_keyboard *next;
};

int bl_keyboard_getchar(void);

int bl_keyboard_layout_lookup(int);
int bl_keyboard_layout_lookup_shift(int);

bl_status_t bl_keyboard_init(void);

void bl_keyboard_register(struct bl_keyboard *);

void bl_keyboard_controller_register(struct bl_keyboard_controller *);
void bl_keyboard_controller_unregister(struct bl_keyboard_controller *);

#endif

