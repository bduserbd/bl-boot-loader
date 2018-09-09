#include "include/cr0.h"

/*
 * The actual BIOS interrupt function call. Performs switch to real mode.
 */
	.globl	bl_bios_interrupt
	.type	bl_bios_interrupt, @function
bl_bios_interrupt:
	.code32

	/* Should not be zero .. */
	andl	%edx, %edx
	jz	_exit

	/* Interrupt number. */
	movb	%al, int_num

	/* Save whole state. */
	pushfl
	pushw	%ds
	pushw	%es
	pushw	%fs
	pushw	%gs
	pushw	%ss
	pushal

	/* Execute interrupt in real mode. */
	call	bl_real_mode_jump
	.code16

	/* bl_real_mode_jump doesn't change %edx. Still get
	   input registers pointer through stack. */
	movl	20(%esp), %edx

	/* Set registers.*/
	movl	(%edx), %edi
	movl	4(%edx), %esi

	movw	8(%edx), %gs
	movw	10(%edx), %fs
	movw	12(%edx), %es
	movw	14(%edx), %ds

	pushl	16(%edx)
	popfl

	movl	20(%edx), %eax
	movl	24(%edx), %ebx
	movl	28(%edx), %ecx
	movl	32(%edx), %edx

	/* Perform interrupt. */
	.byte	0xcd
int_num:.byte	0

	/* Get output. */
	pushl	%ecx
	movl	28(%esp), %ecx
	andl	%ecx, %ecx
	jnz	_store_output
	popl	%ecx
	jmp	_restore

_store_output:

	movl	%edi, (%ecx)
	movl	%esi, 4(%ecx)

	movw	%gs, 8(%ecx)
	movw	%fs, 10(%ecx)
	movw	%es, 12(%ecx)
	movw	%ds, 14(%ecx)

	movl	%eax, 20(%ecx)
	movl	%ebx, 24(%ecx)
	movl	%edx, 32(%ecx)

	popl	%edx
	movl	%edx, 28(%ecx)

	pushfl
	popl	%edx
	movl	%edx, 16(%ecx)

_restore:
	/* Return to proctected mode. */
	calll	bl_protected_mode_jump
	.code32

	/* Restore original state. */
	popal
	popw	%ss
	popw	%gs
	popw	%fs
	popw	%es
	popw	%ds
	popfl

_exit:
	ret

	.size	bl_bios_interrupt, .-bl_bios_interrupt

