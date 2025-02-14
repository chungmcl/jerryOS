#include "jerryTypes.h"
#include "libfdt.h"

int main() {
  // note that the device tree is in big endian while this CPU is lil endian
  u8* deviceTreeAddress;
  asm volatile (
    "mov %0, x1" // Get the device tree's address out of x1
    : "=r"(deviceTreeAddress) // Output operand
    : // No input operands
    : // No clobbered registers
  );

  if (fdt_check_header(deviceTreeAddress)) {
    // panic
  }

  return 0;
}
