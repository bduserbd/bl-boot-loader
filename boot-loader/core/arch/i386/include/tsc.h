#ifndef BL_TSC_H
#define BL_TSC_H

#include "include/bl-types.h"

static inline bl_uint64_t bl_rdtsc(void)
{
	bl_uint32_t low, high;

	asm volatile("rdtsc" : "=a" (low), "=d" (high));

	return low | ((bl_uint64_t)high) << 32;
}

#endif

