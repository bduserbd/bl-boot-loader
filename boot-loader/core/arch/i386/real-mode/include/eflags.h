#ifndef BL_EFLAGS_H
#define BL_EFLAGS_H

#define BL_X86_EFLAGS_CF	(1 << 0)
#define BL_X86_EFLAGS_FIXED	(1 << 1)
#define BL_X86_EFLAGS_PF	(1 << 2)
#define BL_X86_EFLAGS_AF	(1 << 4)
#define BL_X86_EFLAGS_ZF	(1 << 6)
#define BL_X86_EFLAGS_SF	(1 << 7)
#define BL_X86_EFLAGS_TF	(1 << 8)
#define BL_X86_EFLAGS_IF	(1 << 9)
#define BL_X86_EFLAGS_DF	(1 << 10)
#define BL_X86_EFLAGS_OF	(1 << 11)
#define BL_X86_EFLAGS_IOPL	(3 << 12)
#define BL_X86_EFLAGS_NT	(1 << 14)
#define BL_X86_EFLAGS_RF	(1 << 16)
#define BL_X86_EFLAGS_VM	(1 << 17)
#define BL_X86_EFLAGS_AC	(1 << 18)
#define BL_X86_EFLAGS_VIF	(1 << 19)
#define BL_X86_EFLAGS_VIP	(1 << 20)
#define BL_X86_EFLAGS_ID	(1 << 21)

#endif

