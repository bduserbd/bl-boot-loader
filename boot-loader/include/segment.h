#ifndef BL_SEGMENT_H
#define BL_SEGMENT_H

#include "include/types.h"

/* Get segment selectors */
static inline u16 bl_ds(void)
{
	u16 segment;
	asm volatile("mov %%ds,%0" : "=rm" (segment));
	return segment;
}

static inline u16 bl_es(void)
{
	u16 segment;
	asm volatile("mov %%es,%0" : "=rm" (segment));
	return segment;
}

static inline u16 bl_fs(void)
{
	u16 segment;
	asm volatile("mov %%fs,%0" : "=rm" (segment));
	return segment;
}

static inline u16 bl_gs(void)
{
	u16 segment;
	asm volatile("mov %%gs,%0" : "=rm" (segment));
	return segment;
}

#endif

