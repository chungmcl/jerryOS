ENTRY(_start)


SECTIONS
{
  /* Map jerryOS's ELF to 0x104000.
   * The DTB appears to use range 0x0 -> 0x100000,
   * so might as well place jerryOS a 16KB page further.
  */
  _kernel_bin = 0x200000;
  . = _kernel_bin;

  .rodata : {
    _rodata_start = .;
    *(.rodata*)
    _rodata_end = .;
  }

  .text : {
    _text_start = .;
    KEEP*(.text._start)
    *(.text*)
    _text_end = .;
  }

  .device_tree 0x40000000 : { *(.device_tree) }

  .bss 0x4000a000 (NOLOAD) :
  {
    _bss_start = .;
    *(.bss*)
    *(COMMON)
    _bss_end = .;
  }
}
