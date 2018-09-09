#include "firmware/uefi/include/tables.h"
#include "include/export.h"

void bl_time_setup(void)
{

}

bl_uint64_t bl_time_sleep(bl_uint64_t ms)
{
	efi_status_t status;
	efi_event_t event;
	efi_uintn_t index;

	status = bl_system_table->boot_services->create_event(EFI_EVT_TIMER,
		EFI_TPL_CALLBACK, NULL, NULL, &event);
	if (EFI_FAILED(status))
		return 0;

	/* 100 nanoseconds units. */
	bl_system_table->boot_services->set_timer(event, EFI_TIMER_RELATIVE, ms * 10000);

	status = bl_system_table->boot_services->wait_for_event(1, &event, &index);

	bl_system_table->boot_services->close_event(event);

	if (status)
		return 0;

	return ms;
}
BL_EXPORT_FUNC(bl_time_sleep);

