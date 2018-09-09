#ifndef TYPES_H
#define TYPES_H

#define NULL ((void *)0)

/* Signed types */
typedef signed char		s8;
typedef signed short		s16;
typedef signed int		s32;
typedef signed long long	s64;

typedef s8	int8_t;
typedef s16	int16_t;
typedef s32	int32_t;
typedef s64	int64_t;

/* Unsigned types */
typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;

typedef u8	uint8_t;
typedef u16	uint16_t;
typedef u32	uint32_t;
typedef u64	uint64_t;

/* Unsigned types for relative low level things */
typedef s8	__s8;
typedef s16	__s16;
typedef s32	__s32;
typedef s64	__s64;
typedef signed	__s;

/* Unsigned types for relative low level things */
typedef u8	__u8;
typedef u16	__u16;
typedef u32	__u32;
typedef u64	__u64;
typedef unsigned	__u;

/* Various */
typedef u16	wchar_t;
typedef u32	size_t;
typedef u32	addr_t;
typedef s32	ssize_t;

/* GUID */
struct guid {
	u32	part1;
	u16	part2;
	u16	part3;
	u8	part4[8];
} __attribute__((packed));

#define offsetof(st, m)	((size_t)&(((st *)0)->m))

static inline u16 bytes_swap16(u16 val)
{
	return ((val & 0xff) << 8) | (val >> 8);
}

static inline u32 bytes_swap32(u32 val)
{
	return  ((val & 0xff) << 24) 	    |
		(((val >> 8) & 0xff) << 16) |
		(((val >> 16) & 0xff) << 8) |
		(val >> 24);
}

static inline u64 bytes_swap64(u64 val)
{
	return  ((val & 0xff) << 56)	     |
		(((val >> 8) & 0xff) << 48)  |
		(((val >> 16) & 0xff) << 40) |
		(((val >> 24) & 0xff) << 32) |
		(((val >> 32) & 0xff) << 24) |
		(((val >> 40) & 0xff) << 16) |
		(((val >> 48) & 0xff) << 8)  |
		((val >> 58) & 0xff);
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define le16_to_cpu(val) (val)
#define le32_to_cpu(val) (val)
#define le64_to_cpu(val) (val)
#define be16_to_cpu(val) bytes_swap16(val)
#define be32_to_cpu(val) bytes_swap32(val)
#define be64_to_cpu(val) bytes_swap64(val)

#define cpu_to_le16(val) (val)
#define cpu_to_le32(val) (val)
#define cpu_to_le64(val) (val)
#define cpu_to_be16(val) bytes_swap16(val)
#define cpu_to_be32(val) bytes_swap32(val)
#define cpu_to_be64(val) bytes_swap64(val)
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define le16_to_cpu(val) bytes_swap16(val)
#define le32_to_cpu(val) bytes_swap32(val)
#define le64_to_cpu(val) bytes_swap64(val)
#define be16_to_cpu(val) (val)
#define be32_to_cpu(val) (val)
#define be64_to_cpu(val) (val)

#define cpu_to_le16(val) bytes_swap16(val)
#define cpu_to_le32(val) bytes_swap32(val)
#define cpu_to_le64(val) bytes_swap64(val)
#define cpu_to_be16(val) (val)
#define cpu_to_be32(val) (val)
#define cpu_to_be64(val) (val)
#endif

#endif

