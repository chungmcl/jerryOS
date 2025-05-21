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

  // Set the stack pointer to a specific memory address.
  // QEMU will have RAM start at 0x40080000, and the DTB is 
  // automatically loaded at the start of RAM so we will place 
  // the stack arbitrarily at 0xF000 bytes after the start of RAM.
  ldr x1, =0x4008F000
  mov sp, x1

  // For some reason, changing .text to 0x200000 and .bss to 0x4008F008
  // using link.lds makes QEMU move the DTB to 0x0, despite the docs at
  // https://qemu-project.gitlab.io/qemu/system/arm/virt.html stating
  // "For guests booting as “bare-metal” (any other kind of boot), 
  // the DTB is at the start of RAM (0x4000_0000)."
  // Therefore, we pass the DTB address to main() as 0x0 instead of 0x0
  ldr x1, =0x0

  // Jump to main.c:main
  bl main

.end