SECTIONS
{
  /* Map jerryOS's ELF to 0x104000.
   * The DTB appears to use range 0x0 -> 0x100000,
   * so might as well place jerryOS a 16KB page further.
  */
  _kernel_bin = 0x104000;
  . = _kernel_bin;

  .rodata : {
    _rodata_start = .;
    *(.rodata*)
    _rodata_end = .;
  }

  .text : {
    _text_start = .;
    *(.text)
    _text_end = .;
  }

  /* Place .bss at 0x40002000.
   * Place .kernelRootTable at 0x40000000.
   *
   * .bss is where global variables live,
   * and we need to make sure global variables
   * are mapped to RAM and not the read-only flash memory where
   * the .bss will be mapped into if we don't manually adjust via this linker script.
   * According to https://qemu-project.gitlab.io/qemu/system/arm/virt.html,
   * • Flash memory starts at address 0x0000_0000
   * • RAM starts at 0x4000_0000
   * Since jerryOS will use the 16KB page size option on the Cortex-A710,
   * we'll reserve the first 16KB of memory for both .bss and also the kernel's stack.
   * We'll map .bss to the latter half of the first page, and the stack to the 
   * former half of the first page to avoid the stack overwriting global variables.
   * We'll also place the kernel's root page tables at the start of RAM since 
   * the root page table only has 8 entries of u64s, but needs to be 16KB aligned.
   * Let's hope the kernel's stack never goes back far enough to overwrite the root
   * page table!
  */
  .kernelRootTable 0x40000000 : { *(.kernelRootTable) }
  .bss 0x40002000 (NOLOAD) :
  {
    _bss_start = .;
    *(.bss)
    *(COMMON)
    _bss_end = .;
  }
}
