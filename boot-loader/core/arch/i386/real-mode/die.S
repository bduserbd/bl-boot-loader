/*
 * Die if bad settings !
 */
	.code16
	.globl	bl_die
	.type	bl_die, @function
bl_die:
	hlt
	jmp bl_die
	.size	bl_die, .-bl_die

