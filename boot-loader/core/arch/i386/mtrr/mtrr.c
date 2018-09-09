#include "include/msr.h"
#include "include/export.h"
#include "core/include/memory/heap.h"
#include "core/arch/i386/real-mode/include/cpu.h"

static void bl_mtrr_set_entry(int i, bl_uint64_t ptr, bl_uint64_t size, bl_uint64_t addrmask)
{
	bl_uint64_t new_physbase, new_physmask;

	/* Physical base - WC cache. */
	new_physbase = (ptr & addrmask) | 0x1;

	bl_wrmsr(BL_MSR_MTRR_PHYSBASEN(i), new_physbase & 0xffffffff,
		(bl_uint32_t)(new_physbase >> 32));

	/* Valid range. */
	new_physmask = (~(bl_uint64_t)(size - 1) & addrmask) |
		BL_MSR_MTRR_PHYSMASK_V;

	bl_wrmsr(BL_MSR_MTRR_PHYSMASKN(i), new_physmask & 0xffffffff,
		(bl_uint32_t)(new_physmask >> 32));
}

static int bl_mtrr_check_range_overlap(bl_uint64_t ptr, bl_uint64_t size,
	bl_uint64_t addrmask, bl_uint64_t physbase, bl_uint64_t physmask)
{
	/* No WC. */
	if ((physbase & 0x1) == 0)
		return 1;

	if (ptr < (physbase & ~0xfffULL))
		return 1;

	if (ptr + size > (physbase & ~0xfffULL) + 1 + (addrmask & ~(physmask & ~0xfffULL)))
		return 1;

	return 0;
}

void bl_mtrr_set_write_combine_cache(bl_uint64_t ptr, bl_uint64_t size)
{
        int i, free_entry;
        bl_uint32_t eax, ebx, ecx, edx;
        bl_uint32_t low, high;
        bl_uint8_t vcnt;
	bl_uint64_t addrmask;

        /* MTRR support. */
        bl_cpu_cpuid(BL_CPUID_FUNCTION_0x1, &eax, &ebx, &ecx, &edx);
        if ((edx & BL_CPUID_FUNCTION_0x1_MTRR) == 0)
                return;

        bl_rdmsr(BL_MSR_MTRR, &low, &high);
        if ((low & BL_MSR_MTRR_WC) == 0)
                return;

        vcnt = low & BL_MSR_MTRR_VCNT;
        if (vcnt == 0 || vcnt > 8)
                return;

	/* Alignment requirements. */
	ptr = BL_MEMORY_ALIGN_UP(ptr, 0x1000);

	size = BL_MEMORY_ALIGN_UP(size, 0x1000);
	if (size & (size - 1))
		return;

	if (size > 0x1000)
		if (ptr != BL_MEMORY_ALIGN_UP(ptr, size))
			return;

	/* Max physical memory - according to documentation. */
	bl_cpu_cpuid(BL_CPUID_FUNCTION_0x80000000, &eax, &ebx, &ecx, &edx);
	if (eax >= BL_CPUID_FUNCTION_0x80000008) {
		bl_cpu_cpuid(BL_CPUID_FUNCTION_0x80000008, &eax, &ebx, &ecx, &edx);
		addrmask = (1ULL << (eax & BL_CPUID_FUNCTION_0x80000008_PHYS_ADDR_SIZE)) - 1;
	} else
		addrmask = (1ULL << 36) - 1;

	free_entry = -1;
        for (i = 0; i < vcnt; i++) {
                bl_uint64_t physmask;

                bl_rdmsr(BL_MSR_MTRR_PHYSMASKN(i), &low, &high);
                physmask = ((bl_uint64_t)high << 32) | low;

                /* Valid range. */
                if (physmask & BL_MSR_MTRR_PHYSMASK_V) {
			bl_uint64_t physbase;

			bl_rdmsr(BL_MSR_MTRR_PHYSBASEN(i), &low, &high);
			physbase = ((bl_uint64_t)high << 32) | low;

			if (!bl_mtrr_check_range_overlap(ptr, size, addrmask, physbase,
				physmask))
				return;
                } else
			if (free_entry == -1)
				free_entry = i;
        }

	if (free_entry != -1)
		bl_mtrr_set_entry(free_entry ,ptr, size, addrmask);
}

