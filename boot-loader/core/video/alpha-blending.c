#include "core/include/video/fb.h"

bl_video_color_t bl_alpha_blend_color(bl_video_color_t color1, bl_video_color_t color2,
	bl_uint8_t alpha, int *ignore)
{
	bl_uint8_t r1, g1, b1;
	bl_uint8_t r2, g2, b2;

	*ignore = 0;

	if (alpha == 0x0) {
		*ignore = 1;
		return 0;
	} else if (alpha == 0xff)
		return color1;

	r1 = bl_fb_get_red(color1);
	g1 = bl_fb_get_green(color1);
	b1 = bl_fb_get_blue(color1);

	r2 = bl_fb_get_red(color2);
	g2 = bl_fb_get_green(color2);
	b2 = bl_fb_get_blue(color2);

	return bl_fb_prepare_color
		((r1 * ((alpha << 8) + alpha) + r2 * (((alpha ^ 0xff) << 8) + (alpha ^ 0xff))) >> 16,
		(g1 * ((alpha << 8) + alpha) + g2 * (((alpha ^ 0xff) << 8) + (alpha ^ 0xff))) >> 16,
		(b1 * ((alpha << 8) + alpha) + b2 * (((alpha ^ 0xff) << 8) + (alpha ^ 0xff))) >> 16,
		alpha);
}

