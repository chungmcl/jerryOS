ENTRY(_start)


SECTIONS
{
  .device_tree 0x0 : { *(.device_tree) }
  
  .kernel_root_tables 0x40000000 : { *(.kernel_root_tables) }

  .bss 0x4000a000 (NOLOAD) :
  {
    _bss_start = .;
    *(.bss*)
    *(COMMON)
    _bss_end = .;
  }

  _kernel_bin = 0x4000c000;
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
}
