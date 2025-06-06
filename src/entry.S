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

  // Disable trapping of "instructions that access Advanced SIMD and floating-point registers."
  // i.e., enable Advanced SIMD and floating-point registers.
  // CPACR_EL1's FPEN[21:20] bits should be set to
  // "0b11:	This control does not cause execution of any instructions to be trapped"
  mrs x1, CPACR_EL1
  orr x1, x1, #(0b11 << 20)
  msr CPACR_EL1, x1
  isb

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
  // • QEMU maps RAM to physical address 0x4000_0000, and places the DTB there.
  // • jerryOS uses the Cortex-A710's 16KB memory page option.
  // • jerryOS will use the second 16KB page after the DTB for the stack and the kernel's
  //   global variables (the .bss section of the kernel ELF).
  // • The second 16KB page in memory, starting at 0x40008000, is structured such that:
  //   .bss = [0x4000bfff:0x4000a000] (i.e., the second half of the page).
  //   Therefore, sp starts before 0x4000a000 and grows backwards
  //   towards the ROM address range, which is just the jerryOS .text and .rodata, 
  //   to avoid overwriting .bss or any other important data.
  // • On Cortex-A710, sp must be 16-byte aligned, so start 16 bytes before
  //   the first byte of .bss: 0x4000a000 - 0x10 = 0x40009ff0.
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
