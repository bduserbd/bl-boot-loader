#include "include/time.h"
#include "core/include/shell/shell.h"
#include "core/include/gui/gui-app.h"
#include "core/include/loader/module.h"
#include "core/include/video/video.h"
#include "core/include/video/print.h"
#include "core/include/keyboard/keyboard.h"
#include "core/include/usb/usb.h"
#include "core/include/storage/storage.h"

bl_status_t bl_init(void)
{
	bl_status_t status;

	bl_time_setup();

	/* Load modules - dependency is not supported. */
	status = bl_loader_init();
	if (status)
		return status;

	/* Set up relevant components. */
	bl_video_driver_probe();

	bl_gui_app_setup();

	bl_print_str("[1] Setting USB.\n");
	bl_usb_setup();

	bl_print_str("[2] Checking storage devices.\n");
	status = bl_storage_probe();
	if (status) {
		bl_print_str("Unproper storage settings.\n");
		return status;
	}

#if 0
	bl_print_str("[3] Selecting input device.\n");
	status = bl_keyboard_init();
	if (status) {
		bl_print_str("Unproper input device.\n");
		return status;
	}
#endif

	bl_print_str("Initialization complete !\n");

	/* Jump to shell. */
	bl_shell_entry_point();

	return BL_STATUS_SUCCESS;
}

