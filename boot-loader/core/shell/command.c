#include "include/string.h"
#include "core/include/shell/command.h"
#include "core/include/memory/heap.h"
#include "core/include/video/print.h"

struct bl_command_info {
	const char *command;
	const char *help;
	int (*execute)(int, char **);
};

// Keep this list updated
#define BL_COMMAND_TOTAL	6

static struct bl_command_info bl_commands[BL_COMMAND_TOTAL] = {
	{
		.command = "help",
		.help = "Display helpful information about shell commands.",
		.execute = bl_command_help,
	},

	{
		.command = "true",
		.help = "Returns successfull result.",
		.execute = bl_command_true,
	},

	{
		.command = "false",
		.help = "Returns unsuccessfull result.",
		.execute = bl_command_false,
	},

	{
		.command = "last-result",
		.help = "Show the result of the last executed command.",
		.execute = bl_command_last_result,
	},

	{
		.command = "pci-list",
		.help = "Display current attached PCI devices.",
		.execute = bl_command_pci_list,
	},

	{
		.command = "usb-list",
		.help = "Display attached USB devices.",
		.execute = bl_command_usb_list,
	},

#if 0
	{
		.command = "storage-list",
		.help = "Display attached storage devices.",
		.execute = bl_shell_usb_list,
	},
#endif
};

/* Should be enough . */
#define BL_ARGUMENTS	4

static char bl_arguments[BL_ARGUMENTS][BL_SHELL_BUFFER_LENGTH + 1];
static char *bl_argv[BL_ARGUMENTS];

void bl_command_setup(void)
{
	int i;

	bl_memset(bl_arguments, 0, BL_ARGUMENTS * (BL_SHELL_BUFFER_LENGTH + 1));

	for (i = 0; i < BL_ARGUMENTS; i++)
		bl_argv[i] = bl_arguments[i];
}

void bl_command_run(const char *command)
{
	int i, argc;
	bl_size_t len;
	char *copy_command, *part;

	static const char *bl_command_delimiter = " ";

	len = bl_strlen(command);
	if (!len)
		return;

	copy_command = bl_strndup(command, len);
	if (!copy_command)
		return;

	argc = 0;
	part = bl_strtok(copy_command, bl_command_delimiter);

	while (part) {
		if (argc == BL_ARGUMENTS)
			break;

		bl_strncpy(bl_arguments[argc++], part, BL_SHELL_BUFFER_LENGTH);

		part = bl_strtok(NULL, bl_command_delimiter);
	}

	for (i = 0; i < BL_COMMAND_TOTAL; i++)
		if (!bl_strncmp(bl_argv[0], bl_commands[i].command, BL_SHELL_BUFFER_LENGTH)) {
			bl_print_str("\n");
			bl_last_result = bl_commands[i].execute(argc, bl_argv);
			return;
		}

	bl_heap_free(copy_command, len);
}

