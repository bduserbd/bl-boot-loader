#include "include/a20.h"
#include "include/io.h"

static void bl_a20_fast(void)
{
	bl_uint8_t n;

	n = bl_inb(0x92);
	n |= 2;
	n &= 0xfe;
	bl_outb(n, 0x92);
}

int bl_a20_enable(void)
{
	bl_a20_fast();

	return 0;
}

