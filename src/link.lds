SECTIONS
{
  /* Map jerryOS's ELF to 0x200000.
   * The DTB appears to use range 0x0 -> 0x100000,
   * so might as well place jerryOS +0x100000 further.
  */
  . = 0x200000;

  /* Place .bss at 0x4008F008.
   * .bss is where global variables live,
   * and we need to make sure global variables
   * are mapped to RAM and not the read-only flash memory where
   * the .bss will be mapped into by default.
   * According to https://qemu-project.gitlab.io/qemu/system/arm/virt.html,
   * • Flash memory starts at address 0x0000_0000
   * • RAM starts at 0x4000_0000
   * Furthermore, since we set the stack pointer to 0x4008F000 in start.asm,
   * We'll map .bss 8 bytes (for alignment) above it (since the stack grows down).
  */
  .bss 0x4008F008 (NOLOAD) :
  {
    *(.bss)
    *(COMMON)
  }
}
