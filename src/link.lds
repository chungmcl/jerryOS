SECTIONS
{
  /* QEMU's basic bootloader will jump to this address for the binary passed in as -kernel */
  . = 0x0000000040080000;
  .text : { *(.text) }
  .rodata.str1.1 : { *(.rodata.str1.1) }

  /*
  I don't think I technically need these to be here
  .debug_info : { *(.debug_info) }
  .debug_abbrev : { *(.debug_abbrev) }
  .debug_aranges : { *(.debug_aranges) }
  .debug_line : { *(.debug_line) }
  .debug_line_str : { *(.debug_line_str) }
  .debug_str_offsets : { *(.debug_str_offsets) }
  .debug_str : { *(.debug_str) }
  .debug_addr : { *(.debug_addr) }
  .comment : { *(.comment) }
  .symtab : { *(.symtab) }
  .shstrtab : { *(.shstrtab) }
  .strtab : { *(.strtab) }
  */

  /* move eh_frame_hdr somewhere else because it defaults to being before .text, which we don't want */
  .eh_frame_hdr : { *(.eh_frame_hdr) }
}