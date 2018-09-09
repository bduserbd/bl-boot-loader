#ifndef BL_VIDEO_H
#define BL_VIDEO_H

#include "include/bl-types.h"
#include "include/error.h"
#include "core/include/video/edid.h"

/* When EDID is not supported (usually emulators & virtualization).  */
#define BL_VIDEO_DEFAULT_WIDTH	1024
#define BL_VIDEO_DEFAULT_HEIGHT	768

#define BL_VIDEO_DEFAULT_RED_MASK_SIZE		5
#define BL_VIDEO_DEFAULT_GREEN_MASK_SIZE	6
#define BL_VIDEO_DEFAULT_BLUE_MASK_SIZE		5
#define BL_VIDEO_DEFAULT_RESERVED_MASK_SIZE	0

typedef bl_uint32_t bl_video_color_t;

struct bl_video_color_mask {
	bl_uint8_t position;
	bl_uint8_t mask_size;
};

struct bl_video_info {
	/* Screen size */
	bl_uint32_t width;
	bl_uint32_t height;
	bl_uint32_t pitch;

	/* Video memory area */
	bl_addr_t frame_buffer;
	bl_size_t video_memory_size;

	/* Pixel definitions */
	bl_uint8_t bits_per_pixel;
	bl_uint8_t bytes_per_pixel;

	/* Color definitions */
	struct bl_video_color_mask red;
	struct bl_video_color_mask green;
	struct bl_video_color_mask blue;
	struct bl_video_color_mask reserved;
};

typedef enum {
	BL_VIDEO_TYPE_INVALID = -1,
	BL_VIDEO_TYPE_VBE_GRAPHICS,
	BL_VIDEO_TYPE_GOP_GRAPHICS,
} bl_video_t;

struct bl_video_driver {
	bl_video_t type;

	bl_status_t (*init)(void);

	void (*uninit)(void);

	bl_status_t (*set_video)(void);

	bl_status_t (*get_info)(struct bl_video_info *);

	struct bl_video_driver *next;
};

bl_status_t bl_video_driver_probe(void);

bl_uint32_t bl_video_get_width(void);
bl_uint32_t bl_video_get_height(void);
bl_uint32_t bl_video_get_bpp(void);

void bl_video_blit(void *, bl_uint32_t, bl_uint32_t, bl_uint32_t, bl_uint32_t,
        bl_uint32_t, bl_uint32_t);

void bl_video_fill_rectangle_ptr(void *, bl_video_color_t,
	bl_uint32_t, bl_uint32_t, bl_uint32_t, bl_uint32_t);

void bl_video_fill_rectangle(bl_video_color_t, bl_uint32_t, bl_uint32_t,
	bl_uint32_t, bl_uint32_t);

void bl_video_buffer_to_video(bl_uint32_t, bl_uint32_t, bl_uint32_t, bl_uint32_t);

void bl_video_update_from_double_buffer(void);

void bl_video_driver_register(struct bl_video_driver *);
void bl_video_driver_unregister(struct bl_video_driver *);

#endif

