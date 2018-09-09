#include "include/descriptors.h"
#include "include/cr0.h"

/* Set GDT entry. */
	.macro	BL_GDT_ENTRY base=0 limit=0 type=0 s=0 p=0 db=0 g=0

	.word	(\limit & 0xffff)
	.word	(\base & 0xffff)
	.byte	((\base >> 16) & 0xff)
	.byte	(\p << 7) | (\s << 4) | (\type)
	.byte	(\g << 7) | (\db << 6) | ((\limit >> 16) & 0xf)
	.byte	((\base >> 24) & 0xff)

	.endm

/* GDT entries & descriptor. */
	.p2align	3
bl_gdt:
	BL_GDT_ENTRY
	BL_GDT_ENTRY base=0 limit=0xfffff type=0xb s=1 p=1 db=1 g=1
	BL_GDT_ENTRY base=0 limit=0xfffff type=0x3 s=1 p=1 db=1 g=1
	BL_GDT_ENTRY base=0 limit=0xfffff type=0xb s=1 p=1 db=0 g=0
	BL_GDT_ENTRY base=0 limit=0xfffff type=0x3 s=1 p=1 db=0 g=0

	.p2align	3
	.globl	bl_gdt_desc
	.type	bl_gdt_desc, @object
bl_gdt_desc:
	.word	.-bl_gdt-1
	.long	bl_gdt

/* Protected mode IDT. */
	.p2align	3
	.globl	bl_protected_mode_idt
	.type	bl_protected_mode_idt, @object
bl_protected_mode_idt:
	.word	0x0
	.long	0x0

/* Real mode IVT.  */
	.p2align	3
	.globl	bl_real_mode_ivt
	.type	bl_real_mode_ivt, @object
bl_real_mode_ivt:
	.word	0x400
	.long	0x0

/*
 * Jump from protected to real mode.
 */
	.code32
	.globl	bl_real_mode_jump
	.type	bl_real_mode_jump, @function
bl_real_mode_jump:

	sidt	bl_protected_mode_idt
	lidt	bl_real_mode_ivt
	
	/* Set real mode data segments. */
	movw	$BL_GDT_DATA_SEGMENT_REAL_MODE, %bx
	movw	%bx, %ds
	movw	%bx, %es
	movw	%bx, %fs
	movw	%bx, %gs
	movw	%bx, %ss

	/* Clear PE flag and jump to real mode. */
	ljmp	$BL_GDT_CODE_SEGMENT_REAL_MODE, $bl_real_mode_setup
bl_real_mode_setup:
	.code16

	movl	%cr0, %ebx
	andl	$(~BL_X86_CR0_PE), %ebx
	movl	%ebx, %cr0

	ljmpl	$0x0, $bl_in_real_mode

bl_in_real_mode:
	xorl	%ebx, %ebx
	movw	%bx, %ds
	movw	%bx, %es
	movw	%bx, %fs
	movw	%bx, %gs
	movw	%bx, %ss

	retl

	.size	bl_real_mode_jump, .-bl_real_mode_jump

/*
 * Jump from real to protected mode.
 */
	.code16
	.globl	bl_protected_mode_jump
	.type	bl_protected_mode_jump, @function
bl_protected_mode_jump:
	cli

	/* GDT. */
	lgdtl	bl_gdt_desc

	/* Set protection enable flag. */
	movl	%cr0, %ebx
	orl	$BL_X86_CR0_PE, %ebx
	movl	%ebx, %cr0

	/* Long jump. */
	.byte	0x66, 0xea
	.long	bl_in_protected_mode
	.word	0x8

	.code32
bl_in_protected_mode:
	/* Set protocted mode data segments. */
	movw	$BL_GDT_DATA_SEGMENT_PROTECTED_MODE, %bx
	movw	%bx, %ds
	movw	%bx, %es
	movw	%bx, %fs
	movw	%bx, %gs
	movw	%bx, %ss

	sidt	bl_real_mode_ivt
	lidt	bl_protected_mode_idt

	ret

	.size	bl_protected_mode_jump, .-bl_protected_mode_jump

