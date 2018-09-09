/*
 * Call initialization function which executes in protected mode.
 */
	.code16

        .globl  bl_main
        .type   bl_main, @function
bl_main:

	calll	bl_protected_mode_jump

	.code32
	call	bl_sys_main

bl_hlt:
        hlt
        jmp     bl_hlt

        .size bl_main, . - bl_main

