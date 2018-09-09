#include "include/export.h"

int bl_log2(unsigned n)
{
	int i;

	if (!n)
		return -1;

	/* Should be power of 2. */
	if (n & (n - 1))
		return -1;

	for (i = 0; (n & 1) == 0; i++)
		n >>= 1;

	return i;
}
BL_EXPORT_FUNC(bl_log2);

