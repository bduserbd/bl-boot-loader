#ifndef BL_BIOS_H
#define BL_BIOS_H

#include "include/bl-types.h"

struct bl_bios_registers {
	union {
		struct {
			bl_uint32_t edi;
		};
		struct {
			bl_uint16_t di;
		};
	};

	union {
		struct {
			bl_uint32_t esi;
		};
		struct {
			bl_uint16_t si;
		};
	};

	bl_uint16_t gs;
	bl_uint16_t fs;
	bl_uint16_t es;
	bl_uint16_t ds;

	bl_uint32_t eflags;

	union {
		struct {
			bl_uint32_t eax;
		};
		struct {
			bl_uint16_t ax;
		};
		struct {
			bl_uint8_t al, ah;
		};
	};

	union {
		struct {
			bl_uint32_t ebx;
		};
		struct {
			bl_uint16_t bx, hbx;
		};
		struct {
			bl_uint8_t bl, bh;
		};
	};

	union {
		struct {
			bl_uint32_t ecx;
		};
		struct {
			bl_uint16_t cx, hcx;
		};
		struct {
			bl_uint8_t cl, ch;
		};
	};

	union {
		struct {
			bl_uint32_t edx;
		};
		struct {
			bl_uint16_t dx, hdx;
		};
		struct {
			bl_uint8_t dl, dh;
		};
	};
} __attribute__((packed));

#define BL_BIOS_FAR_PTR_TO_ADDRESS(ptr)	\
	(((ptr & 0xffff0000) >> 12) + (ptr & 0xffff))

#define BL_BIOS_PM_TO_RM_SEGMENT(ptr)	\
	((ptr & 0xffff0000) >> 4)

#define BL_BIOS_PM_TO_RM_OFFSET(ptr)	\
	(ptr & 0xffff)

void bl_bios_init_registers(struct bl_bios_registers *);
void bl_bios_set_video_mode(unsigned char);
void bl_bios_print_str(const char *);
void bl_bios_interrupt(bl_uint16_t, struct bl_bios_registers *, struct bl_bios_registers *);

#endif

