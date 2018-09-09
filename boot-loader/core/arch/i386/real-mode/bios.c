#include "include/bios.h"
#include "include/eflags.h"
#include "include/segment.h"
#include "include/string.h"
#include "include/export.h"

/* For the bl_bios_interrupt() function defined in bios-interrupt.S */
BL_EXPORT_FUNC(bl_bios_interrupt);

void bl_bios_init_registers(struct bl_bios_registers *regs)
{
	bl_memset(regs, 0, sizeof(struct bl_bios_registers));

	regs->eflags |= BL_X86_EFLAGS_CF;
}
BL_EXPORT_FUNC(bl_bios_init_registers);

void bl_bios_set_video_mode(unsigned char mode)
{
	struct bl_bios_registers iregs;

	bl_bios_init_registers(&iregs);
	iregs.ah = 0x0;
	iregs.al = mode;

	bl_bios_interrupt(0x10, &iregs, 0);
}
BL_EXPORT_FUNC(bl_bios_set_video_mode);

static void bl_bios_print_char(int c)
{
	struct bl_bios_registers iregs;

	bl_bios_init_registers(&iregs);
	iregs.bx = 0x0007;
	iregs.cx = 0x0001;
	iregs.ah = 0x0e;
	iregs.al = c;

	bl_bios_interrupt(0x10, &iregs, 0);	
}

void bl_bios_print_str(const char *s)
{
	int i;

	for (i = 0; s[i]; i++)
		if (s[i] == '\n') {
			bl_bios_print_char('\r');
			bl_bios_print_char('\n');
		} else
			bl_bios_print_char(s[i]);
}

