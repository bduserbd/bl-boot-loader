#include "core/include/shell/command.h"
#include "core/include/usb/usb.h"

int bl_command_usb_list(int argc, char *argv[])
{
	bl_usb_dump();

	return 0;
}

