.text
.global _start

_start:
  // Grab some system registers for funsies 🤪
  mrs x2, CurrentEL
  lsr x2, x2, #2  // Extract EL number according to formula (right shift 2)
  mrs x3, ID_AA64PFR0_EL1
  mrs x4, ID_AA64MMFR1_EL1
  mrs x5, ID_AA64PFR1_EL1
  mrs x6, ID_AA64MMFR3_EL1
  mrs x7, ID_AA64MMFR4_EL1

  // TODO(chungmcl): Set the register pointer (VBAR_EL1)
  // to an Exception Vector Table
  // mov x8, {Exception Vector Table Addy}
  // msr VBAR_EL1, x8

  // For some reason, changing the address of .text and .bss 
  // using link.lds makes QEMU move the DTB to 0x0, despite the docs at
  // https://qemu-project.gitlab.io/qemu/system/arm/virt.html stating
  // "For guests booting as “bare-metal” (any other kind of boot), 
  // the DTB is at the start of RAM (0x4000_0000)."
  // Therefore, we pass the DTB address to main() as 0x0 instead of 0x4000_0000
  ldr x1, =0x0

  // Set the stack pointer to a specific memory address in RAM.
  // Leave the value in x2 so we can pass the initial value of $sp to main()
  // Why this particular address?
  // • QEMU maps RAM to physical address 0x4000_0000.
  // • jerryOS uses the Cortex-A710's 16KB memory page option.
  // • jerryOS uses the first 16KB page for the stack and the kernel's
  //   global variables (the .bss section of the kernel ELF).
  // • The first 16KB page in memory is structured such that:
  //   [0x40004000:0x40002000] (i.e., the second half of the page) = .bss --
  //   therefore, sp starts before 0x40002000 and grows backwards
  //   towards the ROM address range, which is just the jerryOS .text and .rodata, 
  //   to avoid overwriting .bss or any other important data.
  // • On Cortex-A710, sp must be 16-byte aligned, so start 16 bytes before
  //   the first byte of .bss: 0x40002000 - 0x10 = 0x40001FF0.
  ldr x2, =0x40001FF0
  mov sp, x2

  // Jump to main.c:main
  bl main

.end