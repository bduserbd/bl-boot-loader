#include "core/include/shell/command.h"
#include "core/include/storage/storage.h"

int shell_storage_list(int argc, char *argv[])
{
	bl_storage_dump_devices();

	return 0;
}

