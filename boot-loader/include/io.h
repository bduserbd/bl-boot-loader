#ifndef BL_IO_H
#define BL_IO_H

#include "include/bl-types.h"

/* Basic port I/O */
static inline void bl_outb(u8 v, u16 port)
{
	asm volatile("outb %0,%1" : : "a" (v), "dN" (port));
}

static inline u8 bl_inb(u16 port)
{
	u8 v;
	asm volatile("inb %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

static inline void bl_outw(u16 v, u16 port)
{
	asm volatile("outw %0,%1" : : "a" (v), "dN" (port));
}

static inline u16 bl_inw(u16 port)
{
	u16 v;
	asm volatile("inw %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

static inline void bl_outl(u32 v, u16 port)
{
	asm volatile("outl %0,%1" : : "a" (v), "dN" (port));
}

static inline u32 bl_inl(u16 port)
{
	u32 v;
	asm volatile("inl %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

/* Delay port I/O, usually takes 1ms to complete */
static inline void bl_io_delay(void)
{
	const u16 delay_port = 0x80;
	asm volatile("outb %%al,%0" : : "dN" (delay_port));
}

#endif

