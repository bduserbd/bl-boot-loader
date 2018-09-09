/* Related constants. */
.equ	L_SECTOR_SIZE, 0x200
.equ    L_BL_LOADER_ADDRESS, 0x2000
.equ    L_BL_START_ADDRESS, L_BL_LOADER_ADDRESS + L_SECTOR_SIZE

.equ    L_SP_START,     0x2000 /* Keep from the other side of boot loader */

.equ    L_BL_LOADER_SECTOR, 0x0f
.equ    L_BL_SECTOR, 0x10
.equ    L_BL_SECTOR_COUNT, 0x64

