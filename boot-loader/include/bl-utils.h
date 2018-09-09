#ifndef BL_UTILS_H
#define BL_UTILS_H

#include "include/utils.h"

#define BL_MIN	MIN
#define BL_MAX	MAX

int bl_log2(unsigned);
void bl_divmod64(bl_uint64_t, bl_uint64_t, bl_uint64_t *, bl_uint64_t *);

#endif

