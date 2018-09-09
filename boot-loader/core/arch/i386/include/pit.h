#ifndef BL_PIT_H
#define BL_PIT_H

#include "include/bl-types.h"

/*
 * Programmable Interval Timer (Intel 8253/8254):
 * ----------------------------------------------
 * Operates in 3 channels, at frequency of 1.193182MHz, and uses a counter
 * of 16 bits (although data access is limited to 8 bits).
 * 
 * Channel 0:
 * ----------
 * Connected directly to the PIC chip, on IRQ0.
 *
 * Channel 1:
 * ----------
 * Used to refresh the DRAM.
 *
 * Channel 2:
 * ----------
 * Connected to the PC speaker, to the frequency of the output
 * determines the frequency of the sound produced by the speaker.
 */

/* I/O ports of PIT channels. */
#define BL_PIT_CHANNEL_0_DATA_PORT	0x40
#define BL_PIT_CHANNEL_1_DATA_PORT	0x41
#define BL_PIT_CHANNEL_2_DATA_PORT	0x42

/* Structure of the PIT command register:
 * --------------------------------------
 *
 * |+++++++++|++++++++|++++++++++++++|++++++++|
 * | Channel | Access |Operating mode|Encoding|
 * |+++++++++|++++++++|++++++++++++++|++++++++|
 * |  7 - 6  |  5 - 4 |     3 - 1    |    0   |
 * |+++++++++|++++++++|++++++++++++++|++++++++|
 *
 * Channel specifies the operating channel:
 * ----------------------------------------
 * 1) 00 - Channel 0.
 * 2) 01 - Channel 1.
 * 3) 10 - Channel 2.
 * 4) 11 - Read back command (8254 only)
 *
 * Access mode specifies:
 * ----------------------
 * 1) 00 - Latch count value command.
 * 2) 01 - Access mode: lobyte only.
 * 3) 10 - Access mode: hibyte only.
 * 4) 11 - Access mode: lobyte/hibyte.
 *
 * Operating mode:
 * ---------------
 * 1) 000 - Mode 0 (interrupt on terminal count).
 * 2) 001 - Mode 1 (hardware re-triggerable one-shot).
 * 3) 010 - Mode 2 (rate generator).
 * 4) 011 - Mode 3 (square mode generator).
 * 5) 100 - Mode 4 (software triggered strobe).
 * 6) 101 - Mode 5 (hardware triggered strobe).
 * 7) 110 - Mode 2 (rate generator, same as Mode 2).
 * 8) 111 - Mode 3 (square wave generator, same as Mode 3).
 *
 * Encoding:
 * ---------
 * 1) 0 - 16 bit binary.
 * 2) 1 - four digit BCD.
 *
 */

/* PIT mode/command register I/O port */
#define BL_PIT_COMMAND_REGISTER		0x43
/* Flags of the PIT command register */
#define BL_PIT_COMMAND_REGISTER_CHANNEL_0	(0 << 6)
#define BL_PIT_COMMAND_REGISTER_CHANNEL_1	(1 << 6)
#define BL_PIT_COMMAND_REGISTER_CHANNEL_2	(2 << 6)
#define BL_PIT_COMMAND_REGISTER_READ_BACK	(3 << 6)

#define BL_PIT_COMMAND_REGISTER_ACCESS_MODE_LATCH_COUNT	(0 << 4)
#define BL_PIT_COMMAND_REGISTER_ACCESS_MODE_LOW_BYTE_ONLY	(1 << 4)
#define BL_PIT_COMMAND_REGISTER_ACCESS_MODE_HIGH_BYTE_ONLY	(2 << 4)
#define BL_PIT_COMMAND_REGISTER_ACCESS_MODE_LOW_HIGH_BYTES	(3 << 4)

/* Structure of PIT timer channel controlling port (0x61) 8 bits:
 * -----------------++-------------------------------------------
 *
 * |++++++++++++++++|+++++++++++++++++|+++++++++++++|+++++++++++++|
 * |RAM parity error|I/O channel error| OUT2 status | OUT1 status |
 * |++++++++++++++++|+++++++++++++++++|+++++++++++++|+++++++++++++|
 * | 7 (Read only)  |  6 (Read only ) |5 (Read only)|4 (Read only)|
 * |++++++++++++++++|+++++++++++++++++|+++++++++++++|+++++++++++++|
 *
 * |++++++++++++++++++|+++++++++++++++++++|++++++++++++++++++|++++++++++++++++++|
 * |RAM pa.chk enabled| I/O check enabled |    SPKR control  |   GATE2 control  |
 * |++++++++++++++++++|+++++++++++++++++++|++++++++++++++++++|++++++++++++++++++|
 * | 3 (Read & write) |  2 (Read & write) | 1 (Read & write) | 0 (Read & write) |
 * |++++++++++++++++++|+++++++++++++++++++|++++++++++++++++++|++++++++++++++++++|
 */
#define BL_PIT_CONTROL_PORT	0x61
/* Its flags .. */
#define BL_PIT_CONTROL_PORT_GATE2_ON	0x01
#define BL_PIT_CONTROL_PORT_SPKR_ON	0x02
#define BL_PIT_CONTROL_PORT_OUT1_ON	0x10
#define BL_PIT_CONTROL_PORT_OUT2_ON	0x20

extern void bl_pit_channel2_sleep(bl_uint16_t);

#endif

