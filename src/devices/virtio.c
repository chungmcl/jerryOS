#include "virtio.h"
#include "virtioBlk.h"

bool setupBlockDevice(virtioRegs* regs, u32 interruptID) {
  volatile virtio_blk_config *config = (virtio_blk_config*)regs->Config;
  
}

bool setupVirtIODevice(const void* deviceTreeAddress, s32 virtIODevNodeOffset) {
  s32 regLen;
  const fdt32_t* regPtr = fdt_getprop(deviceTreeAddress, virtIODevNodeOffset, "reg", &regLen);
  virtioRegs* regs = (virtioRegs*)((u64)fdt32_to_cpu(regPtr[1]));

  s32 interruptsLen;
  const fdt32_t* interruptsPtr = fdt_getprop(deviceTreeAddress, virtIODevNodeOffset, "interrupts", &interruptsLen);
  u32 interruptID = (u32)fdt32_to_cpu(interruptsPtr[1]) + 32; // Idk why we have to add a constant 32...?

  u32 magicValue = READ32(regs->MagicValue);
  if (READ32(regs->MagicValue) != VIRTIO_MAGIC) {
    // Wrong magic value
    return false;
  }
  u32 version = READ32(regs->Version);
  if (READ32(regs->Version) != VIRTIO_VERSION) {
    // Wrong VirtIO version
    return false;
  }

  WRITE32(regs->Status, 0);
  dsb(SY);

  WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_ACKNOWLEDGE);
  dsb(SY);

  WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_DRIVER);
  dsb(SY);
      
  u32 deviceID = READ32(regs->DeviceID);
  switch (deviceID) {
    case VIRTIO_DEV_BLK: {
      setupBlockDevice(regs, interruptID);
    }
    default: {
      // Unsupported VirtIO device
    }
  }

  return true;
}
