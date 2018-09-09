#ifndef BL_GUI_PRINT_H
#define BL_GUI_PRINT_H

#include "include/bl-types.h"

void bl_print_str(const char *);

void bl_print_hex(bl_uint32_t);
void bl_print_hex64(bl_uint64_t);

void bl_print_decimal(bl_uint32_t);
void bl_print_decimal64(bl_uint64_t);

#endif

