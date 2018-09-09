#include "core/include/video/screen.h"
#include "include/export.h"

static unsigned char *bl_vidmem = (unsigned char *)0xb8000;
static int bl_vidport = 0x3d4;
static int bl_rows = 0, bl_cols = 0;

void bl_screen_puts(const char *s)
{
	int pos;
	char c;

	while ((c = *s++) != '\0') {
		if (c == '\n') {
			bl_cols = 0;
			bl_rows++;
		} else {
			bl_vidmem[(bl_cols + 80 * bl_rows) * 2] = c;
			if (++bl_cols >= 80) {
				bl_cols = 0;
				bl_rows++;
			}
		}
	}

	pos = (bl_cols + 80 * bl_rows) * 2;
	bl_outb(14, bl_vidport);
	bl_outb(0xff & (pos >> 9), bl_vidport+1);
	bl_outb(15, bl_vidport);
	bl_outb(0xff & (pos >> 1), bl_vidport+1);
}
BL_EXPORT_FUNC(bl_screen_puts);

void bl_screen_puthex(bl_uint64_t number)
{
	char alpha[2] = "0";
	int was_not_zero = 0, bits;
	bl_uint8_t digit;

	if (number == 0) {
		bl_screen_puts(alpha);
		return;
	}

	for (bits = sizeof(number) * 8 - 4; bits >= 0; bits -= 4) {
		digit = (number >> bits) & 0xf;

		if (digit == 0) {
			if (!was_not_zero)
				continue;
		} else
			was_not_zero = 1;

		if (digit < 0xA)
			alpha[0] = '0' + digit;
		else
			alpha[0] = 'a' + (digit - 0xA);

		bl_screen_puts(alpha);
	}
}
BL_EXPORT_FUNC(bl_screen_puthex);

void bl_screen_puts_field(const char *s, const char *info)
{
	bl_screen_puts(s);
	bl_screen_puts(" : ");
	bl_screen_puts(info);
	bl_screen_puts("\n");
}
BL_EXPORT_FUNC(bl_screen_puts_field);

void bl_screen_puthex_field(const char *s, bl_uint64_t number)
{
	bl_screen_puts(s);
	bl_screen_puts(" : ");
	bl_screen_puthex(number);
	bl_screen_puts("\n");
}
BL_EXPORT_FUNC(bl_screen_puthex_field);

