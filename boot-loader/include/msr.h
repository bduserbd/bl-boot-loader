#ifndef BL_MSR_H
#define BL_MSR_H

#include "include/bl-types.h"

enum {
	BL_MSR_MTRR		= 0xfe,
	BL_MSR_MTRR_PHYSBASE	= 0x200,
	BL_MSR_MTRR_PHYSMASK	= 0x201,
};

#define BL_MSR_MTRR_PHYSBASEN(n)	\
	(BL_MSR_MTRR_PHYSBASE + n * 2)

#define BL_MSR_MTRR_PHYSMASKN(n)	\
	(BL_MSR_MTRR_PHYSMASK + n * 2)

enum {
	BL_MSR_MTRR_VCNT	= 0xff,
	BL_MSR_MTRR_FIX		= (1 << 8),
	BL_MSR_MTRR_WC		= (1 << 10),
	BL_MSR_MTRR_SMRR	= (1 << 11),
};

enum {
	BL_MSR_MTRR_PHYSMASK_V	= (1 << 11),
};

static inline void bl_rdmsr(bl_uint32_t msr, bl_uint32_t *low, bl_uint32_t *high)
{
	asm volatile("rdmsr" : "=a"(*low), "=d"(*high) : "c"(msr));
}

static inline void bl_wrmsr(bl_uint32_t msr, bl_uint32_t low, bl_uint32_t high)
{
	asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

void bl_mtrr_set_write_combine_cache(bl_uint64_t, bl_uint64_t);

#endif

