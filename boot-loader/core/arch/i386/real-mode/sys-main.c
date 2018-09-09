#include "include/bios.h"
#include "include/cpu.h"
#include "include/a20.h"
#include "core/include/video/print.h"
#include "core/include/loader/module.h"
#include "core/include/memory/heap.h"

void bl_die(void);
void bl_init(void);
void bl_print_set_output_callback(void (*)(const char *));

extern bl_uint8_t __bl_modules_start[];
extern struct bl_module_list_header *bl_mod_list;

void bl_sys_main(void)
{
	bl_bios_set_video_mode(0x3);

	bl_print_set_output_callback(bl_bios_print_str);

	if (bl_cpu_valid()) {
		bl_print_str("CPU is invalid for OS.\n");
		goto _exit;
	}

	bl_a20_enable();

	bl_heap_init();

	bl_mod_list = (struct bl_module_list_header *)__bl_modules_start;

	bl_init();

_exit:
	bl_die();
}

