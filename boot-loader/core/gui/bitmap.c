#include "include/string.h"
#include "core/include/gui/bitmap.h"
#include "core/include/video/fb.h"
#include "core/include/memory/heap.h"

static void bl_bitmap_set_background_image_default(void)
{
	bl_uint32_t i, j;
	bl_uint32_t x, y;

	x = bl_fb_get_width();
	y = bl_fb_get_height();

	for (j = 0; j < 3; j++)
		for (i = 0; i < 32; i++)
			bl_video_fill_rectangle(bl_fb_prepare_color
				(j == 0 ? 0x7f : i * 0x2,
				j == 1 ? 0x7f : i * 0x1,
				j == 2 ? 0x7f : i * 0x1, 0x7f),
				i * (x / 32), j * (y / 3), x / 32, y / 3);
}

bl_status_t bl_bitmap_set_background(void **ptr)
{
	if (!ptr)
		return BL_STATUS_INVALID_PARAMETERS;

	*ptr = bl_heap_alloc(bl_video_get_width() * bl_video_get_height() *
		bl_video_get_bpp());
	if (!*ptr)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	bl_fb_set_render_ptr(*ptr);

	/* TODO: First try to set image file as background. */

	bl_bitmap_set_background_image_default();

	return BL_STATUS_SUCCESS;
}

static void bl_bitmap_blit_shell_background(bl_uint32_t *x, bl_uint32_t *y,
	bl_uint32_t *w, bl_uint32_t *h)
{
	int dummy;
	bl_uint32_t i, j;
	bl_video_color_t color;

	*x = bl_video_get_width() / 20;
	*y = bl_video_get_height() / 20;

	*w = bl_video_get_width() * 9 / 10;
	*h = bl_video_get_height() * 9 / 10;
	*h = *h - *h % BL_FONT_CHARACTER_HEIGHT;

	for (i = 0; i < *w; i++)
		for (j = 0; j < *h; j++) {
			color = bl_alpha_blend_color(bl_fb_prepare_color(0xf, 0xf, 0xf, 0xf),
					bl_fb_get_pixel(i + *x, j + *y), 0xa0, &dummy);

			/* Unnecessary here .. */
			/* if (dummy)
				continue; */

			bl_fb_set_pixel(color, i + *x, j + *y);
		}
}

void bl_bitmap_set_shell_dimensions(bl_uint32_t *x, bl_uint32_t *y,
	bl_uint32_t *w, bl_uint32_t *h)
{
	bl_bitmap_blit_shell_background(x, y, w, h);
}

static void bl_bitmap_blit_prompt_background(bl_uint32_t *x, bl_uint32_t *y,
	bl_uint32_t *w, bl_uint32_t *h)
{
	int dummy;
	bl_uint32_t i, j;
	bl_video_color_t color;

	*x = bl_video_get_width() / 20;
	*y = bl_video_get_height() * 16 / 20;

	*w = bl_video_get_width() * 9 / 10;
	*h = bl_video_get_height() / 10;

	for (i = 0; i < *w; i++)
		for (j = 0; j < *h; j++) {
			color = bl_alpha_blend_color(bl_fb_prepare_color(0xf, 0xf, 0xf, 0xf),
					bl_fb_get_pixel(i + *x, j + *y), 0xa0, &dummy);

			/* Unnecessary here .. */
			/* if (dummy)
				continue; */

			bl_fb_set_pixel(color, i + *x, j + *y);
		}
}

void bl_bitmap_set_prompt_dimensions(bl_uint32_t *x, bl_uint32_t *y,
	bl_uint32_t *w, bl_uint32_t *h)
{
	bl_bitmap_blit_prompt_background(x, y, w, h);
}

void bl_bitmap_reset_page_vector(struct bl_gui_shell_content *page,
	bl_uint32_t rows, bl_uint32_t cols)
{
	if (!page || !page->vector)
		return;

	bl_memset(page->vector, 0, (rows + 1) * cols);

	page->updated_line.line = page->updated_line.column = 0;
	page->current_line.line = page->current_line.column = 0;
}

bl_status_t bl_bitmap_init_page_vector(struct bl_gui_shell_content *page,
	bl_uint32_t rows, bl_uint32_t cols)
{
	page->vector = bl_heap_alloc((rows + 1) * cols);
	if (!page->vector)
		return BL_STATUS_MEMORY_ALLOCATION_FAILED;

	bl_bitmap_reset_page_vector(page, rows, cols);

	return BL_STATUS_SUCCESS;
}

