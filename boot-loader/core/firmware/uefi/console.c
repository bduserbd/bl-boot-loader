#include "core/firmware/uefi/include/tables.h"

void bl_print_set_output_callback(void (*)(const char *));

static void bl_efi_output_string(efi_wchar_t *s)
{
	if (EFI_FAILED(bl_system_table->con_out->test_string(bl_system_table->con_out, s)))
		return;

	bl_system_table->con_out->output_string(bl_system_table->con_out, s);
}

static void bl_efi_console_print_str(const char *s)
{
	int i;
	efi_wchar_t c[2];

	c[1] = 0;

	for (i = 0; s[i]; i++) {
		c[0] = s[i];

		if (s[i] == '\n')
			bl_efi_output_string(L"\r\n");
		else
			bl_efi_output_string(c);
	}
}

void bl_efi_set_console(void)
{
	bl_system_table->con_out->reset(bl_system_table->con_out, EFI_TRUE);

	bl_print_set_output_callback(bl_efi_console_print_str);
}

