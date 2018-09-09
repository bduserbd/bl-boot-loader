/*
 * Entry point of the EFI application.
 */
	.section .text

# 8(%esp) - EFI system table.
# 4(%esp) - Image handle.
	.type	_start, @function
_start:
	pushl	%ebp
	movl	%esp, %ebp

	pushl	12(%ebp)
	pushl	8(%ebp)

	/* Get our load address. */
	call	1f
1:	popl	%eax
	movl	%eax, %ebx

	/* Relocate us. As we supply PE offsets & information by
           compiling it, %eax points to the MZ DOS header.  */
	addl	$(__bl_image_start-1b), %eax
	addl	$(_DYNAMIC-1b), %ebx
	pushl	%ebx
	pushl	%eax
	call	bl_efi_relocate_self
	addl	$0x8, %esp

	testl	%eax, %eax
	jnz	_exit

	/* Call main. */
	call	bl_main

_exit:
	leave
	ret

	.size	_start, . - _start

