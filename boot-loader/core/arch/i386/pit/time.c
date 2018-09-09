#include "include/time.h"
#include "include/tsc.h"
#include "include/pit.h"
#include "include/export.h"

static bl_uint64_t ticks_per_ms = 0;

void bl_time_setup(void)
{
	bl_uint64_t start, diff;

	start = bl_rdtsc();
	bl_pit_channel2_sleep(0x9f41); // About 32 ms.
	diff = bl_rdtsc() - start;

	ticks_per_ms = diff >> 5;
}

bl_uint64_t bl_time_sleep(bl_uint64_t ms)
{
	bl_uint64_t end;

	end = bl_rdtsc() + ms * ticks_per_ms;

	while (bl_rdtsc() < end) ;

	return ms;
}
BL_EXPORT_FUNC(bl_time_sleep);

