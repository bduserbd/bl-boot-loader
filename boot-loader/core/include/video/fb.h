#ifndef BL_FB_H
#define BL_FB_H

#include "core/include/video/video.h"

bl_status_t bl_fb_init(struct bl_video_driver *);

bl_video_color_t bl_fb_get_pixel_ptr(void *, bl_uint32_t, bl_uint32_t);
bl_video_color_t bl_fb_get_pixel(bl_uint32_t, bl_uint32_t);
void bl_fb_set_pixel(bl_video_color_t, bl_uint32_t, bl_uint32_t);

bl_video_color_t bl_fb_prepare_color(bl_uint8_t, bl_uint8_t, bl_uint8_t, bl_uint8_t);
bl_uint8_t bl_fb_get_red(bl_video_color_t);
bl_uint8_t bl_fb_get_green(bl_video_color_t);
bl_uint8_t bl_fb_get_blue(bl_video_color_t);
bl_uint8_t bl_fb_get_reserved(bl_video_color_t);

void bl_fb_blit_glyph(char, void *, bl_uint32_t, bl_uint32_t);

void bl_fb_update_from_double_buffer(void);

void bl_fb_set_render_ptr(void *);
void bl_fb_switch_from_renderer(void);

void bl_fb_blit(void *, bl_uint32_t, bl_uint32_t, bl_uint32_t, bl_uint32_t,
	bl_uint32_t, bl_uint32_t);

bl_uint32_t bl_fb_get_width(void);
bl_uint32_t bl_fb_get_height(void);
bl_uint32_t bl_fb_get_bpp(void);

void bl_fb_fill_rectangle(bl_video_color_t, bl_uint32_t, bl_uint32_t,
	bl_uint32_t, bl_uint32_t);

bl_video_color_t bl_alpha_blend_color(bl_video_color_t, bl_video_color_t, bl_uint8_t,
	int *);

#endif

