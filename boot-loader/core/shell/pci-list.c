#include "core/include/shell/command.h"
#include "core/include/pci/pci.h"

int bl_command_pci_list(int argc, char *argv[])
{
	bl_pci_dump_devices();

	return 0;
}

