#include "include/string.h"
#include "include/export.h"
#include "core/include/gui/font.h"
#include "core/include/video/fb.h"
#include "core/include/memory/heap.h"

#ifdef FIRMWARE_BIOS
#include "include/msr.h"
#endif

static struct {
	struct bl_video_info info;

	void *renderer;

	struct {
		bl_uint32_t x, y;
		bl_uint32_t w, h;
	} dirty_area;

	void *double_buffer;
} bl_fb;

#define COLOR_MASK(v, sz)	(v & ((1 << sz) - 1))
#define COLOR_LSHIFT(v, pos)	(v << pos)
#define COLOR_RSHIFT(v, pos)	(v >> pos)

bl_video_color_t bl_fb_prepare_color(bl_uint8_t r, bl_uint8_t g, bl_uint8_t b,
	bl_uint8_t reserved)
{
	return  COLOR_LSHIFT(COLOR_MASK(r, bl_fb.info.red.mask_size),
			bl_fb.info.red.position) |
		COLOR_LSHIFT(COLOR_MASK(g, bl_fb.info.green.mask_size),
			bl_fb.info.green.position) |
		COLOR_LSHIFT(COLOR_MASK(b, bl_fb.info.blue.mask_size),
			bl_fb.info.blue.position) |
		COLOR_LSHIFT(COLOR_MASK(reserved, bl_fb.info.reserved.mask_size),
			bl_fb.info.reserved.position);
}

bl_uint8_t bl_fb_get_red(bl_video_color_t color)
{
	return COLOR_MASK(COLOR_RSHIFT(color, bl_fb.info.red.position),
		bl_fb.info.red.mask_size);
}

bl_uint8_t bl_fb_get_green(bl_video_color_t color)
{
	return COLOR_MASK(COLOR_RSHIFT(color, bl_fb.info.green.position),
		bl_fb.info.green.mask_size);
}

bl_uint8_t bl_fb_get_blue(bl_video_color_t color)
{
	return COLOR_MASK(COLOR_RSHIFT(color, bl_fb.info.blue.position),
		bl_fb.info.blue.mask_size);
}

bl_uint8_t bl_fb_get_reserved(bl_video_color_t color)
{
	return COLOR_MASK(COLOR_RSHIFT(color, bl_fb.info.reserved.position),
		bl_fb.info.reserved.mask_size);
}

static void *bl_fb_get_target_ptr(void)
{
	if (bl_fb.renderer)
		return bl_fb.renderer;
	else if (bl_fb.double_buffer)
		return bl_fb.double_buffer;
	else
		return NULL;
}

#define BL_FB_INVALID_POS	(bl_uint32_t)~0

static void bl_fb_reset_dirty_area(void)
{
	bl_fb.dirty_area.x =
	bl_fb.dirty_area.y =
	bl_fb.dirty_area.w =
	bl_fb.dirty_area.h = BL_FB_INVALID_POS;
}

static void bl_fb_update_dirty_area(bl_uint32_t x, bl_uint32_t y, bl_uint32_t w, bl_uint32_t h)
{
	bl_uint32_t dirty_x, dirty_y;
	bl_uint32_t dirty_w, dirty_h;

	dirty_x = bl_fb.dirty_area.x;
	dirty_y = bl_fb.dirty_area.y;
	dirty_w = bl_fb.dirty_area.w;
	dirty_h = bl_fb.dirty_area.h;

	if (x < dirty_x || dirty_x == BL_FB_INVALID_POS)
		bl_fb.dirty_area.x = x;

	if (y < dirty_y || dirty_y == BL_FB_INVALID_POS)
		bl_fb.dirty_area.y = y;

	if (dirty_w == BL_FB_INVALID_POS)
		bl_fb.dirty_area.w = w;
	else {
		if (x < dirty_x) {
			if ((x + w) < (dirty_x + dirty_w))
				bl_fb.dirty_area.w = dirty_x + dirty_w - x;
			else
				bl_fb.dirty_area.w = w;
		} else {
			if (x + w > dirty_x + dirty_w)
				bl_fb.dirty_area.w = x + w - dirty_x;
		}
	}

	if (dirty_h == BL_FB_INVALID_POS)
		bl_fb.dirty_area.h = h;
	else {
		if (y < dirty_y) {
			if (y + h < dirty_y + dirty_h)
				bl_fb.dirty_area.h = dirty_y + dirty_h - y;
			else
				bl_fb.dirty_area.h = h;
		} else {
			if (y + h > dirty_y + dirty_h)
				bl_fb.dirty_area.h = y + h - dirty_y;
		}
	}

	/*bl_print_hex(bl_fb.dirty_area.x); bl_print_str(" ");
	bl_print_hex(bl_fb.dirty_area.y); bl_print_str(" ");
	bl_print_hex(bl_fb.dirty_area.w); bl_print_str(" ");
	bl_print_hex(bl_fb.dirty_area.h); bl_print_str("\n");
*/
}

void bl_fb_blit_glyph(char c, void *blit_bg, bl_uint32_t x, bl_uint32_t y)
{
	int i, j;
	bl_uint8_t *glyph;
	bl_uint8_t *src;
	bl_uint8_t *dst;
	bl_video_color_t white;

	if (c < 0x20)
		return;

	bl_fb_update_dirty_area(x, y, BL_FONT_CHARACTER_WIDTH, BL_FONT_CHARACTER_HEIGHT);

	glyph = (bl_uint8_t *)&bl_font[(int)c];
	src = (bl_uint8_t *)blit_bg + y * bl_fb.info.pitch + x * bl_fb.info.bytes_per_pixel;
	dst = (bl_uint8_t *)bl_fb_get_target_ptr() + y * bl_fb.info.pitch +
		x * bl_fb.info.bytes_per_pixel;

	white = bl_fb_prepare_color(0xff, 0xff, 0xff, 0xff);

	switch (bl_fb.info.bytes_per_pixel) {
	case 2:
		for (i = 0; i < BL_FONT_CHARACTER_HEIGHT; i++) {
			bl_uint64_t fg = ((bl_uint64_t)white << 48) |
					((bl_uint64_t)white << 32) |
					((bl_uint64_t)white << 16) | white;

			bl_uint64_t mask1 = bl_font_lookup_table2[(glyph[i] >> 4) & 0xf];
			bl_uint64_t mask2 = bl_font_lookup_table2[glyph[i] & 0xf];

			*(bl_uint64_t *)dst = (fg & mask1) | (
				(
				(((bl_uint64_t)bl_fb_get_pixel_ptr(src, 0, 0)) << 0) |
				(((bl_uint64_t)bl_fb_get_pixel_ptr(src, 1, 0)) << 16) |
				(((bl_uint64_t)bl_fb_get_pixel_ptr(src, 2, 0)) << 32) |
				(((bl_uint64_t)bl_fb_get_pixel_ptr(src, 3, 0)) << 48)
				) & ~mask1);

			*(bl_uint64_t *)(dst + 8) = (fg & mask2) | (
				(
				(((bl_uint64_t)bl_fb_get_pixel_ptr(src, 4, 0)) << 0) |
				(((bl_uint64_t)bl_fb_get_pixel_ptr(src, 5, 0)) << 16) |
				(((bl_uint64_t)bl_fb_get_pixel_ptr(src, 6, 0)) << 32) |
				(((bl_uint64_t)bl_fb_get_pixel_ptr(src, 7, 0)) << 48)
				) & ~mask2);

			src += bl_fb.info.pitch;
			dst += bl_fb.info.pitch;
		}

		break;

	case 3:
		// TODO: Improve this.
		for (i = 0; i < BL_FONT_CHARACTER_HEIGHT; i++) {
			int w = BL_FONT_CHARACTER_WIDTH - 1;
			for (j = w; j >= 0; j--) {
				if (glyph[i] & (1 << j)) {
					((bl_uint8_t *)dst + (w - j) * 3)[0] = 0xff;
					((bl_uint8_t *)dst + (w - j) * 3)[1] = 0xff;
					((bl_uint8_t *)dst + (w - j) * 3)[2] = 0xff;
				} else {
					bl_video_color_t c;

					c = bl_fb_get_pixel_ptr(src, 0, 0);

					((bl_uint8_t *)dst + (w - j) * 3)[0] = c & 0xff;
					((bl_uint8_t *)dst + (w - j) * 3)[1] = (c >> 8) & 0xff;
					((bl_uint8_t *)dst + (w - j) * 3)[2] = (c >> 16) & 0xff;
				}
			}

			src += bl_fb.info.pitch;
			dst += bl_fb.info.pitch;
		}

		break;

	case 4:
		for (i = 0; i < BL_FONT_CHARACTER_HEIGHT; i++) {
			bl_uint64_t fg = ((bl_uint64_t)white << 32) | white;

			bl_uint64_t mask1 = bl_font_lookup_table4[(glyph[i] >> 6) & 0x3];
			bl_uint64_t mask2 = bl_font_lookup_table4[(glyph[i] >> 4) & 0x3];
			bl_uint64_t mask3 = bl_font_lookup_table4[(glyph[i] >> 2) & 0x3];
			bl_uint64_t mask4 = bl_font_lookup_table4[glyph[i] & 0x3];

			*(bl_uint64_t *)dst = (fg & mask1) |
				((((bl_uint64_t)bl_fb_get_pixel_ptr(src, 0, 0)) << 32 |
						 bl_fb_get_pixel_ptr(src, 1, 0)) & ~mask1);

			*(bl_uint64_t *)(dst + 8) = (fg & mask2) |
				((((bl_uint64_t)bl_fb_get_pixel_ptr(src, 2, 0)) << 32 |
						 bl_fb_get_pixel_ptr(src, 3, 0)) & ~mask2);

			*(bl_uint64_t *)(dst + 16) = (fg & mask3) |
				((((bl_uint64_t)bl_fb_get_pixel_ptr(src, 4, 0)) << 32 |
						 bl_fb_get_pixel_ptr(src, 5, 0)) & ~mask3);

			*(bl_uint64_t *)(dst + 24) = (fg & mask4) |
				((((bl_uint64_t)bl_fb_get_pixel_ptr(src, 6, 0)) << 32 |
						 bl_fb_get_pixel_ptr(src, 7, 0)) & ~mask4);

			src += bl_fb.info.pitch;
			dst += bl_fb.info.pitch;
		}

		break;

	default:
		break;
	}
}

bl_video_color_t bl_fb_get_pixel_ptr(void *src, bl_uint32_t x, bl_uint32_t y)
{
	bl_uint8_t *ptr;

	ptr = (bl_uint8_t *)src + y * bl_fb.info.pitch + x * bl_fb.info.bytes_per_pixel;

	switch (bl_fb.info.bits_per_pixel) {
	case 15:
	case 16:
		return *(bl_uint16_t *)ptr;

	case 24:
		return (((bl_uint8_t *)ptr)[0]) | 
			(((bl_uint8_t *)ptr)[1] << 8) | 
			(((bl_uint8_t *)ptr)[2] << 16);

	case 32:
		return *(bl_uint32_t *)ptr;

	default:
		break;
	}

	return 0;
}

bl_video_color_t bl_fb_get_pixel(bl_uint32_t x, bl_uint32_t y)
{
	return bl_fb_get_pixel_ptr(bl_fb_get_target_ptr(), x ,y);
}

void bl_fb_set_pixel(bl_video_color_t color, bl_uint32_t x, bl_uint32_t y)
{
	bl_uint8_t *dst;

	dst = (bl_uint8_t *)bl_fb_get_target_ptr() + y * bl_fb.info.pitch +
		x * bl_fb.info.bytes_per_pixel;

	switch (bl_fb.info.bits_per_pixel) {
	case 15:
	case 16:
		*(bl_uint16_t *)dst = color & 0xffff;
		break;

	case 24:
		((bl_uint8_t *)dst)[0] = color & 0xff;
		((bl_uint8_t *)dst)[1] = (color >> 8) & 0xff;
		((bl_uint8_t *)dst)[2] = (color >> 16) & 0xff;
		break;

	case 32:
		*(bl_uint32_t *)dst = color;
		break;

	default:
		break;
	}
}

void bl_fb_update_from_double_buffer(void)
{
	int i;
	bl_uint8_t *src, *dst;
	bl_uint32_t x, y, w, h;

	x = bl_fb.dirty_area.x;
	y = bl_fb.dirty_area.y;
	w = bl_fb.dirty_area.w;
	h = bl_fb.dirty_area.h;

	if (x == BL_FB_INVALID_POS || y == BL_FB_INVALID_POS || w == BL_FB_INVALID_POS ||
		h == BL_FB_INVALID_POS)
		return;

	src = (bl_uint8_t *)bl_fb_get_target_ptr() + y * bl_fb.info.pitch + x * bl_fb.info.bytes_per_pixel;
	dst = (bl_uint8_t *)bl_fb.info.frame_buffer + y * bl_fb.info.pitch + x * bl_fb.info.bytes_per_pixel;

	for (i = 0; i < h; i++) {
		bl_memcpy(dst, src, w * bl_fb.info.bytes_per_pixel);
		src += bl_fb.info.pitch;
		dst += bl_fb.info.pitch;
	}

	bl_fb_reset_dirty_area();
}

static void bl_fb_fill_rectangle_16bits(bl_uint16_t *dst, bl_video_color_t color,
	bl_uint32_t pitch, bl_uint32_t w, bl_uint32_t h)
{
	int i, j;

	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++)
			*dst++ = (color & 0xffff);

		dst = (bl_uint16_t *)((char *)dst + pitch);
	}
}
static void bl_fb_fill_rectangle_24bits(bl_uint8_t *dst, bl_video_color_t color,
	bl_uint32_t pitch, bl_uint32_t w, bl_uint32_t h)
{
	int i, j;
	bl_uint8_t c1, c2, c3;

	c1 = (bl_uint8_t)((color >> 0) & 0xff);
	c2 = (bl_uint8_t)((color >> 8) & 0xff);
	c3 = (bl_uint8_t)((color >> 16) & 0xff);

	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
			*dst++ = c1;
			*dst++ = c2;
			*dst++ = c3;
		}

		dst += pitch;
	}
}

static void bl_fb_fill_rectangle_32bits(bl_uint32_t *dst, bl_video_color_t color,
	bl_uint32_t pitch, bl_uint32_t w, bl_uint32_t h)
{
	int i, j;

	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++)
			*dst++ = color;

		dst = (bl_uint32_t *)((char *)dst + pitch);
	}
}

static void bl_fb_fill_rectangle_ptr(void *ptr, bl_video_color_t color,
	bl_uint32_t x, bl_uint32_t y, bl_uint32_t w, bl_uint32_t h)
{
	bl_uint8_t *dst;
	bl_uint32_t pitch;

	if (!ptr)
		return;

	dst = (bl_uint8_t *)ptr + y * bl_fb.info.pitch + x * bl_fb.info.bytes_per_pixel;
	pitch = bl_fb.info.pitch - bl_fb.info.bytes_per_pixel * w;
	
	switch (bl_fb.info.bytes_per_pixel) {
	case 2:
		bl_fb_fill_rectangle_16bits((bl_uint16_t *)dst, color, pitch, w, h);
		break;

	case 3:
		bl_fb_fill_rectangle_24bits(dst, color, pitch, w, h);
		break;

	case 4:
		bl_fb_fill_rectangle_32bits((bl_uint32_t *)dst, color, pitch, w, h);
		break;

	default:
		break;
	}
}

void bl_fb_fill_rectangle(bl_video_color_t color, bl_uint32_t x, bl_uint32_t y,
	bl_uint32_t w, bl_uint32_t h)
{
	void *ptr;

	ptr = bl_fb_get_target_ptr();
	if (!ptr)
		return;

	bl_fb_update_dirty_area(x, y, w, h);

	bl_fb_fill_rectangle_ptr(ptr, color, x, y, w, h);
}

bl_uint32_t bl_fb_get_width(void)
{
	return bl_fb.info.width;
}

bl_uint32_t bl_fb_get_height(void)
{
	return bl_fb.info.height;
}

bl_uint32_t bl_fb_get_bpp(void)
{
	return bl_fb.info.bytes_per_pixel;
}

void bl_fb_set_render_ptr(void *ptr)
{
	if (ptr)
		bl_fb.renderer = ptr;
}

void bl_fb_switch_from_renderer(void)
{
	int i;
	void *ptr;
	bl_uint8_t *src, *dst;
	bl_uint32_t w, h;

	if (!bl_fb.renderer)
		return;

	ptr = bl_fb.renderer;
	bl_fb.renderer = NULL;

	src = (bl_uint8_t *)ptr;
	dst = (bl_uint8_t *)bl_fb_get_target_ptr();

	w = bl_fb_get_width();
	h = bl_fb_get_height();

	for (i = 0; i < h; i++) {
		bl_memcpy(dst, src, w * bl_fb.info.bytes_per_pixel);
		src += bl_fb.info.pitch;
		dst += bl_fb.info.pitch;
	}
}

bl_status_t bl_fb_init(struct bl_video_driver *video)
{
	bl_status_t status;

	if (!video)
		return BL_STATUS_INVALID_PARAMETERS;

	status = video->get_info(&bl_fb.info);
	if (status)
		return status;

#ifdef FIRMWARE_BIOS
	bl_mtrr_set_write_combine_cache(bl_fb.info.frame_buffer,
		bl_fb.info.video_memory_size);
#endif

	bl_fb.renderer = NULL;

	bl_fb_reset_dirty_area();

	bl_fb.double_buffer = bl_heap_alloc(bl_fb_get_width() * bl_fb_get_height() *
		bl_fb_get_bpp());
	if (!bl_fb.double_buffer)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	/*bl_fb_fill_rectangle(0xe77, 0, 0, 500, 500);
	bl_fb_fill_rectangle(0x4111, 250, 250, 500, 500);
	bl_fb_update_from_double_buffer();
*/
	return BL_STATUS_SUCCESS;
}

