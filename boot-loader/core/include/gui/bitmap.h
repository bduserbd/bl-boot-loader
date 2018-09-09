#ifndef BL_BITMAP_H
#define BL_BITMAP_H

#include "include/error.h"
#include "core/include/gui/gui.h"
#include "core/include/gui/font.h"

bl_status_t bl_bitmap_set_background(void **);
void bl_bitmap_set_shell_dimensions(bl_uint32_t *, bl_uint32_t *, bl_uint32_t *, bl_uint32_t *);
void bl_bitmap_set_prompt_dimensions(bl_uint32_t *, bl_uint32_t *, bl_uint32_t *, bl_uint32_t *);

bl_status_t bl_bitmap_init_page_vector(struct bl_gui_shell_content *, bl_uint32_t, bl_uint32_t);
void bl_bitmap_reset_page_vector(struct bl_gui_shell_content *, bl_uint32_t, bl_uint32_t);

#endif

