#ifndef BL_GUI_H
#define BL_GUI_H

#include "include/bl-types.h"

/* Internal structure for keeping shell content in order to perform blitting. */
struct bl_gui_shell_line {
	bl_uint32_t line; // Accounting line number per page.
	bl_uint32_t column;
};

struct bl_gui_shell_content {
	struct bl_gui_shell_line updated_line;
	struct bl_gui_shell_line current_line;

	char *vector;
};

struct bl_gui {
	/* A background image or bitmap. */
	void *background;

	/* Shell comes with consistent bitmap font without kerning, anti-aliasing .. */
	bl_uint32_t shell_x, shell_y;
	bl_uint32_t shell_w, shell_h;

	bl_uint32_t prompt_x, prompt_y;
	bl_uint32_t prompt_w, prompt_h;

	bl_uint32_t rows, cols;

	int should_scroll;
	int scroll;
	bl_uint32_t viewed_lines;

	/* Character vectors. One contains the current shell content
	   that it is full. The other keeps the content being updated.
	   Once the second vector is full, the first one starts
	   accounting updates .. */
        int active;
	struct bl_gui_shell_content page[2];
};

#endif

