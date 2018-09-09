OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(bl_main)

SECTIONS
{
 . = 0x2200;
 __bl_image_start = .;

 bl_main = .;

 .text : { *(.text*) }
 .rodata : { *(.rodata*) }
 .data : { *(.data*) }
 .bss : { *(.bss*) }

 .bl_strtab : { *(.bl_strtab) }

 .bl_symtab : {
  __bl_symtab_start = .;
  *(.bl_symtab)
  __bl_symtab_end = .;
 }

 __bl_modules_start = .;

 /DISCARD/ : {
  *(.comment)
  *(.eh_frame)
 }
}
