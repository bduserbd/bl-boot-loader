	.code16
	.section ".sys_start", "ax"

.include "constants.S"

	.globl	_start
_start:

	ljmp	$0x0, $real_start
real_start:

	cli

	movb	$0xe, %ah
	movb	$'B', %al
	int	$0x10

	/* Check if we boot from hard disk */
	#cmpb	$0x80, %dl
	#jnz	l_die

	/* Set data & segment, segment selectors to 0 */
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %ss

	/* Set %sp */
	movw	$L_SP_START, %sp

	sti

	/* Restore anyway %dl */
	pushw	%dx

	/* Check for extended functions */
	movb	$0x41, %ah
	movw	$0x55aa, %bx
	int	$0x13

	popw	%dx
	pushw	%dx

	jc	l_die
	cmpw	$0xaa55, %bx
	jne	l_die

	andw	$1, %cx
	jz	l_die

	/* Initialize the disk address packet */
	movw	$l_disk_address_packet, %si
	movb	$0x10, (%si)
	movb	$0x0, 1(%si)
	movw	$0x1, 2(%si)
	movw	$L_BL_LOADER_ADDRESS, 4(%si)
	movw	$0x0, 6(%si)
	movl	$L_BL_LOADER_SECTOR, 8(%si)
	movl	$0x0, 12(%si)

	/* Perform disk read */
	movb	$0x42, %ah
	int	$0x13

	/* LBA read is not supported, so exit.  */
	jc	l_die

	/* Jump to the actual loader of the boot loader. */
	movb	$0xe, %ah
	movb	$'A', %al
	int	$0x10

	calll	L_BL_LOADER_ADDRESS

l_die:
	hlt
	jmp	l_die

.include "disk-address-packet.S"

	.=0x200-0x2
	.byte	0x55
	.byte	0xaa

