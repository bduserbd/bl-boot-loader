#include "core/include/shell/command.h"
#include "core/include/video/print.h"

int bl_last_result = 0;

int bl_command_last_result(int argc, char *argv[])
{
	bl_print_hex(bl_last_result);

	return 0;
}

