.text
.global _start

_start:
  // Set the stack pointer to a specific memory address.
  // QEMU will have our kernel's .text start at 0x40080000, so
  // placing the stack arbitrarily at 0xF000 bytes after the start of .text.
  ldr x1, =0x4008F000
  mov sp, x1

  // Device tree address is currently stored in x0. Move it to x1
  // so we can grab it later, since x0 will be overwritten.
  mov x1, x0

  // Jump to main.c:main
  bl main

.end