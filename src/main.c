#include "main.h"
#include "devicesSetup.h"
#include "kevin.h"

s32 main() {
  // note that the device tree is in big endian while this CPU is lil endian
  const void* deviceTreeAddress;
  const void* kernelBinAddress;
  asm volatile (
    "mov %0, x1\n" // Get the device tree's address out of x1
    "mov %1, x2" // Get the kernel binary address out of x2
    : "=r"(deviceTreeAddress), 
      "=r"(kernelBinAddress)
    : // No input operands
    : // No clobbered registers
  );

  hardwareInfo hwInfo;
  hwInfo.kernelBinStartAddr = kernelBinAddress;
  if (!setupDevices(deviceTreeAddress, &hwInfo)) {
    // panic
  }

  u64 memSizeGB = hwInfo.ramLen >> 30;

  if (!setupPTM(&hwInfo)
  ) {
    // panic
  }

  return 0;
}
