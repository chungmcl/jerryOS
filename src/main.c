#include "main.h"
#include "devicesSetup.h"
#include "kevin.h"

s32 main() {
  // note that the device tree is in big endian while this CPU is lil endian
  const void* deviceTreeAddress;
  asm volatile (
    "mov %0, x1" // Get the device tree's address out of x1
    : "=r"(deviceTreeAddress) // Output operand
    : // No input operands
    : // No clobbered registers
  );

  if (!setupDevices(deviceTreeAddress)) {
    // panic
  }

  if (!setupPTM()) {
    // panic
  }

  return 0;
}
