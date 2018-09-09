#include "include/string.h"
#include "core/include/shell/command.h"
#include "core/include/keyboard/keyboard.h"
#include "core/include/storage/storage.h"
#include "core/include/fs/fs.h"
#include "core/include/video/print.h"

static bl_fs_handle_t operated_fs_handle = NULL;

static int disk_index = -1;
static struct bl_storage_device *operated_disk = NULL;

static int partition_index = -1;
static struct bl_partition *operated_partition = NULL;

static void bl_shell_print_prompt(void)
{
	bl_print_str("\n");

	bl_print_str("[");
	if (disk_index != -1)
		bl_print_hex(disk_index);
	bl_print_str("]");

	bl_print_str("[");
	if (partition_index != -1)
		bl_print_hex(partition_index);
	bl_print_str("]");

#define BL_SHELL_PROMPT	("> ")
	bl_print_str(BL_SHELL_PROMPT);
}

static void bl_shell_set_disk(int index)
{
	struct bl_storage_device *disk;

	if (disk_index == index)
		return;

	disk = bl_storage_device_get(index);
	if (!disk)
		return;

	disk_index = index;
	operated_disk = disk;
	operated_partition = NULL;
}

static void bl_shell_set_partition(int index)
{
	struct bl_partition *partition;

	if (operated_disk) {
		if (partition_index == index)
			return;

		partition = bl_storage_partition_get(operated_disk, index);
		if (partition) {
			operated_fs_handle = bl_fs_try_mount(operated_disk, partition);
			if (!operated_fs_handle)
				return;

			partition_index = index;
			operated_partition = partition;

			return;
		}
	}

	operated_partition = NULL;
}

void bl_shell_entry_point(void)
{
#if 0
	int c, len;
	char s[2];
	char buf[BL_SHELL_BUFFER_LENGTH + 1];
#endif
	bl_command_setup();

	//s[1] = '\0';

#define BL_SHELL_ZERO_BUFFER()					\
	do {							\
		len = 0;					\
		bl_memset(buf, 0, BL_SHELL_BUFFER_LENGTH + 1);	\
	} while (0)

	//BL_SHELL_ZERO_BUFFER();

	bl_storage_dump_devices();

	bl_shell_set_disk(1);
	bl_shell_set_partition(1);

	bl_file_open(operated_fs_handle, "/home/");
#if 0
	bl_file_open(operated_fs_handle, "/home/user/grub-2.02/grub-core/video/readers/jpeg.c");
	//bl_file_open(operated_fs_handle, "/New/");
	bl_file_ls(operated_fs_handle, "/grub-2.02/grub-core/net/arp.c");
#endif

	bl_shell_print_prompt();

#if 0
	for (;;) {
		c = bl_keyboard_getchar();
		if (c == -1)
			continue;

		if (c == '\n') {
			if (buf[0]) {
				bl_command_run(buf);
				BL_SHELL_ZERO_BUFFER();
			}

			bl_shell_print_prompt();
			continue;
		}

		if (!bl_isprint(c))
			continue;

		s[0] = (char)c;

		if (len < BL_SHELL_BUFFER_LENGTH)
			buf[len++] = s[0];

		bl_print_str(s);
	}
#endif
}

