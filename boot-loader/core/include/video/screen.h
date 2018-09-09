#ifndef BL_SCREEN_H
#define BL_SCREEN_H

#include "include/bl-types.h"
#include "include/io.h"

void bl_screen_puts(const char *);
void bl_screen_puthex(bl_uint64_t);
void bl_screen_puts_field(const char *, const char *);
void bl_screen_puthex_field(const char *, bl_uint64_t);

#endif

