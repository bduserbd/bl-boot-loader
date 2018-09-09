#ifndef BL_LOADER_MODULES_LOADER
#define BL_LOADER_MODULES_LOADER

#include "include/error.h"

/* Exporting module name & dependencies. */
#define BL_MODULE_MAX_NAME_LEN  64

#define BL_MODULE_NAME(name)                                            \
        static const char __bl_mod_name[BL_MODULE_MAX_NAME_LEN]         \
        __attribute__((section(".bl_mod_name"), used)) = { name }

/* Modules should export only these functions */
#define BL_MODULE_INIT()	void bl_module_initialize(void)
#define BL_MODULE_UNINIT()	void bl_module_uninitialize(void)

#endif

