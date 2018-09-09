#include "include/export.h"
#include "include/bl-utils.h"
#include "core/include/video/print.h"

static void (*bl_print_callback)(const char *) = NULL;

void bl_print_set_output_callback(void (*callback)(const char *))
{
	bl_print_callback = callback;
}

void bl_print_str(const char *str)
{
	if (bl_print_callback)
		bl_print_callback(str);
}
BL_EXPORT_FUNC(bl_print_str);

#define BL_PRINT_HEX							\
        char s[2];							\
        int zero = 0, bits;						\
        bl_uint8_t digit;						\
									\
	s[0] = '0';							\
	s[1] = '\0';							\
									\
        if (number == 0) {						\
		bl_print_str(s);					\
                return;							\
        }								\
									\
        for (bits = sizeof(number) * 8 - 4; bits >= 0; bits -= 4) {	\
                digit = (number >> bits) & 0xf;				\
									\
                if (digit == 0) {					\
                        if (!zero)					\
                                continue;				\
                } else							\
                        zero = 1;					\
									\
                if (digit < 0xA)					\
                        s[0] = '0' + digit;				\
                else							\
                        s[0] = 'a' + digit - 0xA;			\
									\
                bl_print_str(s);					\
        }

void bl_print_hex(bl_uint32_t number)
{
	BL_PRINT_HEX
}
BL_EXPORT_FUNC(bl_print_hex);

void bl_print_hex64(bl_uint64_t number)
{
	BL_PRINT_HEX
}
BL_EXPORT_FUNC(bl_print_hex64);

void bl_print_decimal(bl_uint32_t number)
{
	char s[2];
	bl_uint32_t reverse = 0;

	while (number) {
		reverse = 10  * reverse	+ number % 10;
		number /= 10;
	}

	s[1] = 0;
	while (reverse) {
		s[0] = '0' + reverse % 10;
		bl_print_str(s);

		reverse /= 10;
	}
}
BL_EXPORT_FUNC(bl_print_decimal);

void bl_print_decimal64(bl_uint64_t number)
{
	char s[2];
	bl_uint64_t reverse = 0;
	bl_uint64_t q, r;

	while (number) {
		bl_divmod64(number, 10, &q, &r);

		reverse = 10  * reverse	+ r;
		number = q;
	}

	s[1] = 0;
	while (reverse) {
		bl_divmod64(reverse, 10, &q, &r);

		s[0] = '0' + r;
		bl_print_str(s);

		reverse = q;
	}
}
BL_EXPORT_FUNC(bl_print_decimal64);

