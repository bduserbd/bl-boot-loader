#ifndef BL_TYPES_H
#define BL_TYPES_H

#include "include/types.h"

/* Signed types */
typedef s8	bl_int8_t;
typedef s16	bl_int16_t;
typedef s32	bl_int32_t;
typedef s64	bl_int64_t;

/* Unsigned types */
typedef u8	bl_uint8_t;
typedef u16	bl_uint16_t;
typedef u32	bl_uint32_t;
typedef u64	bl_uint64_t;

/* Various */
typedef wchar_t		bl_wchar_t;
typedef unsigned int	bl_size_t;
typedef unsigned int	bl_addr_t;
typedef bl_uint16_t	bl_port_t;
typedef unsigned long	bl_offset_t;

/* GUID */
typedef struct guid	bl_guid_t;

#endif

