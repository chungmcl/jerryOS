SECTIONS
{
  /* Map jerryOS's ELF to 0x200000.
   * The DTB appears to use range 0x0 -> 0x100000,
   * so might as well place jerryOS a 16KB page further.
  */
  . = 0x104000;

  /* Place .bss at 0x40002000.
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
  */
  .bss 0x40002000 (NOLOAD) :
  {
    *(.bss)
    *(COMMON)
  }
}
