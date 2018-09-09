#include "include/string.h"
#include "core/include/gui/bitmap.h"
#include "core/include/video/fb.h"
#include "core/include/memory/heap.h"

void bl_print_set_output_callback(void (*)(const char *));

static struct bl_gui *bl_gui_app = NULL;

static inline bl_uint32_t bl_gui_app_shell_pos(struct bl_gui_shell_line *line)
{
	return line->line * bl_gui_app->cols + line->column;
}

static inline int bl_gui_app_change_page(bl_uint32_t line_index)
{
	/* NOTE: bl_gui_app->viewed_lines == bl_gui_app->rows !!! */
	return line_index + 1 == bl_gui_app->viewed_lines;
}

static void bl_gui_app_swap_pages(void)
{
	struct bl_gui_shell_content *page;

	bl_gui_app->active = !bl_gui_app->active;
	page = &bl_gui_app->page[bl_gui_app->active];

	bl_bitmap_reset_page_vector(page, bl_gui_app->rows, bl_gui_app->cols);
}

static void bl_gui_app_update_pages(const char *s)
{
	int i;
	struct bl_gui_shell_content *page;

	if (!s || !bl_gui_app || !bl_gui_app->page[0].vector || !bl_gui_app->page[1].vector)
		return;

	page = &bl_gui_app->page[bl_gui_app->active];

	for (i = 0; s[i]; i++) {
		if (page->current_line.column + 1 < bl_gui_app->cols && s[i] != '\n') {
			page->vector[bl_gui_app_shell_pos(&page->current_line)] = s[i];
			page->current_line.column++;
		} else {
			page->current_line.line++;
			page->current_line.column = 0;

			if (page->current_line.line == bl_gui_app->rows) {
				bl_gui_app->should_scroll = 1;

				bl_gui_app_swap_pages();
				page = &bl_gui_app->page[bl_gui_app->active];
			}
		}
	}
}

static void bl_gui_app_clean_shell_screen(void)
{
	struct bl_gui_shell_line start, end;
	bl_uint32_t pos, end_pos;

	start.column = 0;
	start.line = 0;

	end.column = bl_gui_app->cols - 1;
	end.line = bl_gui_app->rows - 1;

	pos = bl_gui_app_shell_pos(&start);
	end_pos = bl_gui_app_shell_pos(&end);

	while (pos < end_pos) {
		bl_fb_blit_glyph(' ', bl_gui_app->background,
			bl_gui_app->shell_x + (pos % bl_gui_app->cols) * BL_FONT_CHARACTER_WIDTH,
			bl_gui_app->shell_y + (pos / bl_gui_app->cols) * BL_FONT_CHARACTER_HEIGHT);

		pos++;

		//if (!page->vector[pos])
		//	pos += bl_gui_app->cols - pos % bl_gui_app->cols;
	}
}

static void bl_gui_app_flush_page_buffers(void)
{
	struct bl_gui_shell_content *page;
	bl_uint32_t pos, end_pos;

	if (!bl_gui_app || !bl_gui_app->page[0].vector || !bl_gui_app->page[1].vector)
		return;

	if (bl_gui_app->should_scroll) {
		bl_gui_app->should_scroll = 0;
		bl_gui_app_clean_shell_screen();
	}

	page = &bl_gui_app->page[bl_gui_app->active];

	pos = bl_gui_app_shell_pos(&page->updated_line);
	end_pos = bl_gui_app_shell_pos(&page->current_line);

	while (pos < end_pos) {
		bl_fb_blit_glyph(page->vector[pos], bl_gui_app->background,
			bl_gui_app->shell_x + (pos % bl_gui_app->cols) * BL_FONT_CHARACTER_WIDTH,
			bl_gui_app->shell_y + (pos / bl_gui_app->cols) * BL_FONT_CHARACTER_HEIGHT);

		pos++;

		if (!page->vector[pos])
			pos += bl_gui_app->cols - pos % bl_gui_app->cols;
	}

	bl_memcpy(&page->updated_line, &page->current_line, sizeof(struct bl_gui_shell_line));
}

static int gui_prepared = 0;

static void bl_gui_app_print_str(const char *s)
{
	bl_gui_app_update_pages(s);
	bl_gui_app_flush_page_buffers();

	if (gui_prepared)
		bl_video_update_from_double_buffer();
}

#if 0
static void bl_gui_app_print_test(void)
{
	int i, j;
	int big;
	char s[4];

	//s[1] = '\0';

	s[2] = '\n';
	s[3] = '\0';

	big = 0;
#define X	33

	for (i = 0; i < 2 * X - 1; i++) {
		if (i && (i % bl_gui_app->rows) == 0)
			big = !big;

		if (big)
			s[0] = s[1] = 'A' + i % 26;
		else
			s[0] = s[1] = 'a' + i % 26;

		//for (j = 0; j < i % (bl_gui_app->rows + 1); j++)
		bl_print_str(s);
		//bl_print_str(s);
		//bl_print_str("\n");
	}
}
#endif

static void bl_gui_app_print_prologue(void)
{
	int i;

#define BOOT_LOADER_PROLOGUE	("~~~Boot Loader Shell~~~\n")

	bl_gui_app_print_str("\n");

	for (i = 0; i < (bl_gui_app->cols - sizeof(BOOT_LOADER_PROLOGUE)) / 2; i++)
		bl_gui_app_print_str(" ");

	bl_gui_app_print_str(BOOT_LOADER_PROLOGUE);
}

void bl_gui_app_setup(void)
{
	int i;
	bl_status_t status;

	if (bl_gui_app)
		return;

	bl_gui_app = bl_heap_alloc(sizeof(struct bl_gui));
	if (!bl_gui_app)
		return;

	/* Try to set background as renderer when we build GUI. */
	status = bl_bitmap_set_background(&bl_gui_app->background);
	if (status)
		return;

	/* Shell & prompt dimensions. Should not overlap ..  */
	bl_bitmap_set_shell_dimensions(&bl_gui_app->shell_x, &bl_gui_app->shell_y,
		&bl_gui_app->shell_w, &bl_gui_app->shell_h);

	//bl_bitmap_set_prompt_dimensions(&bl_gui_app->prompt_x, &bl_gui_app->prompt_y,
	//	&bl_gui_app->prompt_w, &bl_gui_app->prompt_h);

	bl_gui_app->rows = bl_gui_app->shell_h / BL_FONT_CHARACTER_HEIGHT;
	bl_gui_app->cols = bl_gui_app->shell_w / BL_FONT_CHARACTER_WIDTH;

	/* Metadata. */
	bl_gui_app->should_scroll = 0;
	bl_gui_app->scroll = 0;
	bl_gui_app->viewed_lines = 1;

	bl_gui_app->active = 0;
	for (i = 0; i < 2; i++) {
		status = bl_bitmap_init_page_vector(&bl_gui_app->page[i], bl_gui_app->rows,
			bl_gui_app->cols);
		if (status)
			return;
	}

	/* New output callback. */
	bl_print_set_output_callback(bl_gui_app_print_str);

	/* Do this in any case .. */
	bl_fb_switch_from_renderer();

	bl_gui_app_print_prologue();
	//bl_gui_app_print_test();

	gui_prepared = 1;

	bl_video_update_from_double_buffer();
}

