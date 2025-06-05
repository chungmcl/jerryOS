.text
.global _start

_start:
  // Grab some system registers for funsies 🤪
  mrs x16, CurrentEL
  lsr x16, x16, #2  // Extract EL number according to formula (right shift 2)
  mrs x17, ID_AA64PFR0_EL1
  mrs x18, ID_AA64MMFR1_EL1
  mrs x19, ID_AA64PFR1_EL1
  mrs x20, ID_AA64MMFR3_EL1
  mrs x21, ID_AA64MMFR4_EL1

  // TODO(chungmcl): Set the register pointer (VBAR_EL1)
  // to an Exception Vector Table
  // mov x8, {Exception Vector Table Addy}
  // msr VBAR_EL1, x8

  // https://qemu-project.gitlab.io/qemu/system/arm/virt.html states:
  // "For guests booting as “bare-metal” (any other kind of boot), 
  // the DTB is at the start of RAM (0x4000_0000)."
  // We pass to main the address of the device tree via x1
  ldr x1, =0x40000000

  // Set the stack pointer to a specific memory address in RAM.
  // Leave the value in x2 so we can pass the initial value of $sp to main()
  // Why this particular address?
  // • QEMU maps RAM to physical address 0x4000_0000, and places the DTB there.
  // • jerryOS uses the Cortex-A710's 16KB memory page option.
  // • jerryOS will use the second 16KB page after the DTB for the stack and the kernel's
  //   global variables (the .bss section of the kernel ELF).
  // • The second 16KB page in memory, starting at 0x40008000, is structured such that:
  //   .bss = [0x4000c000:0x4000a000] (i.e., the second half of the page).
  //   Therefore, sp starts before 0x4000a000 and grows backwards
  //   towards the ROM address range, which is just the jerryOS .text and .rodata, 
  //   to avoid overwriting .bss or any other important data.
  // • On Cortex-A710, sp must be 16-byte aligned, so start 16 bytes before
  //   the first byte of .bss: 0x40002000 - 0x10 = 0x40009ff0.
  ldr x2, =0x40009ff0
  mov sp, x2

  ldr x3, =_kernel_bin
  ldr x4, =_rodata_start
  ldr x5, =_rodata_end
  ldr x6, =_text_start
  ldr x7, =_text_end
  ldr x8, =_bss_start
  ldr x9, =_bss_end

  // Jump to main.rs:main()
  bl main

.end
