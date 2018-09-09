#ifndef BL_CPU_H
#define BL_CPU_H

#include "include/error.h"
#include "include/bl-types.h"

enum {
	BL_CPUID_FUNCTION_0x0		= 0x0,
	BL_CPUID_FUNCTION_0x1		= 0x1,
	BL_CPUID_FUNCTION_0x80000000	= 0x80000000,
	BL_CPUID_FUNCTION_0x80000008	= 0x80000008,
};

enum {
	BL_CPUID_FUNCTION_0x1_TSC	= (1 << 4),
	BL_CPUID_FUNCTION_0x1_MSR	= (1 << 5),
	BL_CPUID_FUNCTION_0x1_MTRR	= (1 << 12),
};

enum {
	BL_CPUID_FUNCTION_0x80000008_PHYS_ADDR_SIZE	= 0xff,
};

bl_status_t bl_cpu_valid(void);

int bl_cpu_eflag(bl_uint32_t);
void bl_cpu_cpuid(bl_uint32_t, bl_uint32_t *, bl_uint32_t *, bl_uint32_t *, bl_uint32_t *);

#endif

