#include "include/pit.h"
#include "include/io.h"
#include "include/export.h"

void bl_pit_channel2_sleep(bl_uint16_t ticks)
{
        /* Write command */
        bl_outb(BL_PIT_COMMAND_REGISTER_CHANNEL_2 |
                BL_PIT_COMMAND_REGISTER_ACCESS_MODE_HIGH_BYTE_ONLY,
                BL_PIT_COMMAND_REGISTER);

        /* Write data. First low byte, then high */
        bl_outb(ticks & 0xff, BL_PIT_CHANNEL_2_DATA_PORT);
        bl_outb((ticks >> 8) & 0xff, BL_PIT_CHANNEL_2_DATA_PORT);

        while ((bl_inb(BL_PIT_CONTROL_PORT) & BL_PIT_CONTROL_PORT_OUT2_ON) == 0);
}
BL_EXPORT_FUNC(bl_pit_channel2_sleep);

