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

  deviceSetupInfo deviceInfo;
  if (!setupDevices(deviceTreeAddress, &deviceInfo)) {
    // panic
  }

  u64 memSizeGB = deviceInfo.ramLenBytes >> 30;

  if (!setupPTM()) {
    // panic
  }

  return 0;
}
