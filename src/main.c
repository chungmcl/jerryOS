#include "main.h"
#include "libfdt.h"

bool setupDevices(const void* deviceTreeAddress) {
  if (fdt_check_header(deviceTreeAddress) != 0) return false;

  s32 currNodeOffset = 0;
  s32 depth = 0;
  const void* currNode = (const void*)deviceTreeAddress;

  do {
    currNodeOffset = fdt_next_node(currNode, currNodeOffset, &depth);
    if (currNodeOffset < 0) {
      break;
    }

    const char* currNodeName = fdt_get_name(currNode, currNodeOffset, NULL);
    if (strStartsWith(currNodeName, "virtio_mmio")) {
      s32 regLen;
      const fdt32_t* regPtr = fdt_getprop(deviceTreeAddress, currNodeOffset, "reg", &regLen);
      virtioRegs* regs = (virtioRegs*)((u64)fdt32_to_cpu(regPtr[1]));

      s32 interruptsLen;
      const fdt32_t* interruptsPtr = fdt_getprop(deviceTreeAddress, currNodeOffset, "interrupts", &interruptsLen);
      u32 interruptID = (u32)fdt32_to_cpu(interruptsPtr[1]) + 32; // Idk why we have to add a constant 32...?

      WRITE32(regs->Status, 0);
      dsb();

      WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_ACKNOWLEDGE);
      dsb();

      WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_DRIVER);
      dsb();
      
      u32 deviceID = READ32(regs->DeviceID);
      switch (deviceID) {
        case VIRTIO_DEV_BLK: {
          const char* msg = "supported virtio device";
        }
        default: {
          const char* msg = "unsupported virtio device";
        }
      }

    }
  } while (currNodeOffset >= 0);

  return true;
}

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

  return 0;
}
