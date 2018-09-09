	.code16
	.section ".sys_start", "ax"

.include "constants.S"

	.globl	_start2
_start2:

	/* Initialize the disk address packet */
	movw	$l_disk_address_packet, %si
	movb	$0x10, (%si)
	movb	$0x0, 1(%si)
	movw	$0x1, 2(%si)
	movw	$L_BL_START_ADDRESS, 4(%si)
	movw	$0x0, 6(%si)
	movl	$L_BL_SECTOR, 8(%si)
	movl	$0x0, 12(%si)

	/* Read boot loader into memory */
	movw	$0x0, %cx
read_one_sector:

	/* Perform disk read */
	movb	$0x42, %ah
	int	$0x13

	/* LBA read is not supported, so exit.  */
	jc	l_bl_die

	addw	$L_SECTOR_SIZE, 4(%si)
	addw	$0x1, 8(%si)
	addw	$0x1, %cx

	/* Boot loader size is assumed to be limited by this. */
	cmp $L_BL_SECTOR_COUNT, %cx
	jne read_one_sector

	/* Call the start function. */
	calll	L_BL_START_ADDRESS

l_bl_die:
	hlt
	jmp	l_bl_die

.include "disk-address-packet.S"

	.=0x200-0x2
	.byte	0x55
	.byte	0xaa

