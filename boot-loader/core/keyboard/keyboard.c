#include "include/export.h"
#include "core/include/keyboard/keyboard.h"

static struct bl_keyboard *bl_kbds = NULL;
static struct bl_keyboard *bl_active_kbd = NULL;

static struct bl_keyboard_controller *bl_kbd_ctrls = NULL;

int bl_keyboard_getchar(void)
{
	if (bl_active_kbd)
		return bl_active_kbd->controller->getchar(bl_active_kbd);
	else
		return -1;
}

static struct bl_keyboard_controller *bl_keyboard_controller_match_type(bl_keyboard_t type)
{
	struct bl_keyboard_controller *controller;

	controller = bl_kbd_ctrls;
	while (controller) {
		if (controller->type == type)
			return controller;

		controller = controller->next;
	}

	return NULL;
}

static struct bl_keyboard *bl_keyboard_scan_type(bl_keyboard_t type)
{
	bl_status_t status;
	struct bl_keyboard *kbd;
	struct bl_keyboard_controller *controller;

	kbd = bl_kbds;

	while (kbd) {
		if (kbd->type == type) {
			controller = bl_keyboard_controller_match_type(type);

			if (controller && controller->initialize && controller->getchar) {
				kbd->controller = controller;

				status = kbd->controller->initialize(kbd);
				if (!status)
					return kbd;
			}
		}

		kbd = kbd->next;
	}

	return NULL;
}

bl_status_t bl_keyboard_init(void)
{
	struct bl_keyboard *kbd;

	/* Prefer USB keyboard first. */
	kbd = bl_keyboard_scan_type(BL_KEYBOARD_USB);
	if (kbd)
		bl_active_kbd = kbd;
	else {
		kbd = bl_keyboard_scan_type(BL_KEYBOARD_PS2);
		if (kbd)
			bl_active_kbd = kbd;
		else
			return BL_STATUS_UNSUPPORTED;
	}

	return BL_STATUS_SUCCESS;
}

void bl_keyboard_register(struct bl_keyboard *kbd)
{
	kbd->next = bl_kbds;
	bl_kbds = kbd;
}
BL_EXPORT_FUNC(bl_keyboard_register);

void bl_keyboard_unregister(struct bl_keyboard *kbd)
{

}
BL_EXPORT_FUNC(bl_keyboard_unregister);

void bl_keyboard_controller_register(struct bl_keyboard_controller *kbd_ctrl)
{
	kbd_ctrl->next = bl_kbd_ctrls;
	bl_kbd_ctrls = kbd_ctrl;
}
BL_EXPORT_FUNC(bl_keyboard_controller_register);

void bl_keyboard_controller_unregister(struct bl_keyboard_controller *kbd_ctrl)
{

}
BL_EXPORT_FUNC(bl_keyboard_controller_unregister);

