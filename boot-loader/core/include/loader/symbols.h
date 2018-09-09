#ifndef BL_LOADER_MODULES_SYMBOLS
#define BL_LOADER_MODULES_SYMBOLS

#include "include/error.h"
#include "include/bl-types.h"
#include "include/export.h"

bl_status_t bl_symbols_find(const char *, struct bl_export_symbol **);

#endif

