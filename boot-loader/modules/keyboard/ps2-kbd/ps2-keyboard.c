#include "ps2-keyboard.h"
#include "core/include/loader/loader.h"
#include "core/include/keyboard/keyboard.h"
#include "core/include/memory/heap.h"

/* NOTE: Should be used when there is no USB support. */

BL_MODULE_NAME("PS2 Keyboard");

static const bl_uint32_t scan_code_set2[256] = {
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	'\t',			'`',		0,
	0,	0,			BL_KEY_LSHIFT,	0,
	0,	'q',			'1',		0,
	0,	0,			'z',		's',
	'a',	'w',			'2',		0,
	0,	'c',			'x',		'd',
	'e',	'4',			'3',		0,
	0,	' ',			'v',		'f',
	't',	'r',			'5',		0,
	0,	'n',			'b',		'h',
	'g',	'y',			'6',		0,
	0,	0,			'm',		'j',
	'u',	'7',			'8',		0,
	0,	',',			'k',		'i',
	'o',	'0',			'9',		0,
	0,	'.',			'/',		'l',
	';',	'p',			'-',		0,
	0,	0,			'\'',		0,
	'[',	'=',			0,		0,
	0,	BL_KEY_RSHIFT,		'\n',		']',
	0,	'\\',			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
};

static const bl_uint32_t scan_code_set2_with_shift[256] = {
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	'\t',			'`',		0,
	0,	0,			BL_KEY_LSHIFT,	0,
	0,	'Q',			'!',		0,
	0,	0,			'Z',		'S',
	'A',	'W',			'@',		0,
	0,	'C',			'X',		'D',
	'E',	'$',			'#',		0,
	0,	' ',			'V',		'F',
	'T',	'R',			'%',		0,
	0,	'N',			'B',		'H',
	'G',	'Y',			'^',		0,
	0,	0,			'M',		'J',
	'U',	'&',			'*',		0,
	0,	'<',			'K',		'I',
	'O',	')',			'(',		0,
	0,	'>',			'?',		'L',
	':',	'P',			'_',		0,
	0,	0,			'\"',		0,
	'{',	'+',			0,		0,
	0,	BL_KEY_RSHIFT,		'\n',		'}',
	0,	'|',			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
	0,	0,			0,		0,
};

static inline int bl_ps2_keyboard_has_output(void)
{
	return bl_inb(BL_PS2_KEYBOARD_FUNCTION_STATUS_REGISTER) &
		BL_PS2_KEYBOARD_STATUS_REGISTER_OUTPUT_BUFFER_FULL;
}

/* Sends keys as they were pressed */
static int bl_ps2_keyboard_get_key(void)
{
	bl_uint8_t scan_code;
	static int key_released = 0, lshift_pressed = 0, rshift_pressed = 0;

	if (!bl_ps2_keyboard_has_output())
		goto _no_key;

	scan_code = bl_inb(BL_PS2_KEYBOARD_FUNCTION_DATA_OUTPUT_REGISTER);
	if (scan_code == 0xff)
		goto _no_key;

	if (scan_code == 0xf0) {
		key_released = 1;
		goto _no_key;
	}

	unsigned key = scan_code_set2[scan_code];

	if (key & BL_PS2_KEYBOARD_UNPRINTABLE_KEY) {
		switch (key) {
		case BL_KEY_LSHIFT:
			lshift_pressed = !lshift_pressed;
			break;

		case BL_KEY_RSHIFT:
			rshift_pressed = !rshift_pressed;
			break;
		}
	} else if (!key_released)	
		return lshift_pressed || rshift_pressed ?
			scan_code_set2_with_shift[scan_code] : key;	

	key_released = 0;

_no_key:
	return BL_PS2_KEYBOARD_NO_KEY;
}

static int bl_ps2_keyboard_getchar(struct bl_keyboard *kbd)
{
	int c;

	if (kbd->type != BL_KEYBOARD_PS2)
		return -1;

	while (1) {
		c = bl_ps2_keyboard_get_key();
		if (c != BL_PS2_KEYBOARD_NO_KEY && c > 0)
			return c;
	}
}

static void bl_ps2_keyboard_controller_wait_for_input(void)
{
	while (bl_inb(BL_PS2_KEYBOARD_FUNCTION_STATUS_REGISTER) &
		BL_PS2_KEYBOARD_STATUS_REGISTER_INPUT_BUFFER_FULL) ;
}

/* A command that retrieves info. */
static void bl_ps2_keyboard_command(bl_uint8_t command)
{
	bl_outb(command, BL_PS2_KEYBOARD_FUNCTION_SEND_COMMAND);

	/* Wait for command to complete */
	while ((bl_inb(BL_PS2_KEYBOARD_FUNCTION_STATUS_REGISTER) &
		BL_PS2_KEYBOARD_STATUS_REGISTER_OUTPUT_BUFFER_FULL) == 0) ;
}

static void bl_ps2_keyboard_set_general_mode(void)
{
	bl_uint8_t c;

	/* Read config */
	bl_ps2_keyboard_controller_wait_for_input();
	bl_outb(BL_PS2_KEYBOARD_COMMAND_READ_CONFIG, BL_PS2_KEYBOARD_FUNCTION_SEND_COMMAND);

	while ((bl_inb(BL_PS2_KEYBOARD_FUNCTION_STATUS_REGISTER) &
		BL_PS2_KEYBOARD_STATUS_REGISTER_OUTPUT_BUFFER_FULL) == 0) ;

	c = bl_inb(BL_PS2_KEYBOARD_FUNCTION_DATA_OUTPUT_REGISTER);
	c &= ~(1 << 0); // Don't generate interrupt when data is placed for output.
	c &= ~(1 << 6); // Don't use IBM compatibility mode.

	/* Write config */
	bl_ps2_keyboard_controller_wait_for_input();
	bl_outb(BL_PS2_KEYBOARD_COMMAND_WRITE_CONFIG, BL_PS2_KEYBOARD_FUNCTION_SEND_COMMAND);
	bl_outb(c, BL_PS2_KEYBOARD_FUNCTION_DATA_OUTPUT_REGISTER);
}

static bl_status_t bl_ps2_keyboard_interface_test(void)
{
	bl_ps2_keyboard_command(BL_PS2_KEYBOARD_COMMAND_INTERFACE_TEST);

	if (bl_inb(BL_PS2_KEYBOARD_FUNCTION_DATA_OUTPUT_REGISTER))
		return BL_STATUS_FAILURE;
	else
		return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ps2_keyboard_self_test(void)
{
	bl_ps2_keyboard_command(BL_PS2_KEYBOARD_COMMAND_SELF_TEST);

	if (bl_inb(BL_PS2_KEYBOARD_FUNCTION_DATA_OUTPUT_REGISTER) != BL_PS2_KEYBOARD_SELF_TEST_OK)
		return BL_STATUS_FAILURE;
	else
		return BL_STATUS_SUCCESS;
}

static bl_status_t bl_ps2_keyboard_initialize(struct bl_keyboard *kbd)
{
	bl_status_t status;

	status = bl_ps2_keyboard_self_test();
	if (status)
		return status;

	status = bl_ps2_keyboard_interface_test();
	if (status)
		return status;

	bl_ps2_keyboard_set_general_mode();

	return BL_STATUS_SUCCESS;
}

static struct bl_keyboard_controller ps2_keyboard_controller = {
	.type = BL_KEYBOARD_PS2,
	.initialize = bl_ps2_keyboard_initialize,
	.getchar = bl_ps2_keyboard_getchar,
};

BL_MODULE_INIT()
{
	struct bl_keyboard *kbd;

	kbd = bl_heap_alloc(sizeof(struct bl_keyboard));
	if (!kbd)
		return;

	kbd->type = BL_KEYBOARD_PS2;
	kbd->data = NULL;
	kbd->next = NULL;

	bl_keyboard_controller_register(&ps2_keyboard_controller);
	bl_keyboard_register(kbd);
}

BL_MODULE_UNINIT()
{

}

