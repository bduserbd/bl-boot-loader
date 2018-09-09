#include "include/cpu.h"
#include "include/bios.h"
#include "include/cr0.h"
#include "include/eflags.h"
#include "include/msr.h"
#include "include/export.h"

int bl_cpu_eflag(bl_uint32_t flags)
{
	bl_uint32_t a, b;

	asm volatile("pushfl\n"
		"pushfl\n"
		"pop %0\n"
		"mov %0,%1\n"
		"xor %2,%1\n"
		"push %1\n"
		"popfl\n"
		"pushfl\n"
		"pop %1\n"
		"popfl"
		: "=&r" (a), "=&r" (b)
		: "ri" (flags));

	return !!((a ^ b) & flags);
}
BL_EXPORT_FUNC(bl_cpu_eflag);

void bl_cpu_cpuid(bl_uint32_t function, bl_uint32_t *eax, bl_uint32_t *ebx,
	bl_uint32_t *ecx, bl_uint32_t *edx)
{
	asm volatile(".ifnc %%ebx,%3 ; movl  %%ebx,%3 ; .endif  \n\t"
		 "cpuid                                     \n\t"
		 ".ifnc %%ebx,%3 ; xchgl %%ebx,%3 ; .endif  \n\t"
		: "=a" (*eax), "=c" (*ecx), "=d" (*edx), "=b" (*ebx)
		: "a" (function));
}
BL_EXPORT_FUNC(bl_cpu_cpuid);

static int bl_cpu_is_intel(char vendor[12])
{
	return vendor[0] == 'G' && vendor[1] == 'e' && vendor[2] == 'n' &&
		vendor[3] == 'u' && vendor[4] == 'i' && vendor[5] == 'n' &&
		vendor[6] == 'e' && vendor[7] == 'I' && vendor[8] == 'n' &&
		vendor[9] == 't' && vendor[10] == 'e' && vendor[11] == 'l';
}

bl_status_t bl_cpu_valid(void)
{
	char vendor[12];
	bl_uint32_t max_std_function;
	bl_uint32_t eax, ebx, ecx, edx;

	/* EFLAGS. */
	if (!bl_cpu_eflag(BL_X86_EFLAGS_AC) || !bl_cpu_eflag(BL_X86_EFLAGS_ID))
		return BL_STATUS_UNSUPPORTED;

	/* Vendor. */
	bl_cpu_cpuid(BL_CPUID_FUNCTION_0x0, &max_std_function, (bl_uint32_t *)&vendor[0],
		(bl_uint32_t *)&vendor[8], (bl_uint32_t *)&vendor[4]);

	if (!bl_cpu_is_intel(vendor))
		return BL_STATUS_FAILURE;

	/* TSC & MSR support. */
	bl_cpu_cpuid(BL_CPUID_FUNCTION_0x1, &eax, &ebx, &ecx, &edx);

	if ((edx & BL_CPUID_FUNCTION_0x1_TSC) == 0 ||
		(edx & BL_CPUID_FUNCTION_0x1_MSR) == 0)
		return BL_STATUS_UNSUPPORTED;

	return BL_STATUS_SUCCESS;
}

