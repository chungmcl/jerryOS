#include "virtio.h"
#include "virtioBlk.h"

/*
The driver MUST follow this sequence to initialize a device:
  1. Reset the device.
  2. Set the ACKNOWLEDGE status bit: the guest OS has notice the device.
  3. Set the DRIVER status bit: the guest OS knows how to drive the device.
  4. Read device feature bits, and write the subset of feature bits understood by the OS and driver to the
      device. During this step the driver MAY read (but MUST NOT write) the device-specific configuration
      fields to check that it can support the device before accepting it.
  5. Set the FEATURES_OK status bit. The driver MUST NOT accept new feature bits after this step.
  6. Re-read device status to ensure the FEATURES_OK bit is still set: otherwise, the device does not
      support our subset of features and the device is unusable.
  7. Perform device-specific setup, including discovery of virtqueues for the device, optional per-bus setup,
      reading and possibly writing the device’s virtio configuration space, and population of virtqueues.
  8. Set the DRIVER_OK status bit. At this point the device is “live”.
      If any of these steps go irrecoverably wrong, the driver SHOULD set the FAILED status bit to indicate that it
      has given up on the device (it can reset the device later to restart if desired). The driver MUST NOT continue
      initialization in that case.
      The driver MUST NOT notify the device before setting DRIVER_OK.
*/

bool setupBlockDevice(
  virtioRegs* regs, 
  u32 interruptID, 
  virtioCapability capabilities[], 
  u32 capabilitiesLen
) {
  volatile virtioBlkConfig* config = (virtioBlkConfig*)regs->Config;

  u64 devSizeBytes = ((u64)READ32(config->capacity)) * VIRTIO_BLK_BLOCK_SIZE;

  u32 deviceFeatures0 = READ32(regs->DeviceFeatures);
  WRITE32(regs->DeviceFeaturesSel, 1);
  dsb(SY);
  u32 deviceFeatures1 = READ32(regs->DeviceFeatures);

  u32 driverFeatures0 = 0;
  for (u32 i = 0; i < capabilitiesLen; i += 1) {
    u32 bit = capabilities[i].bit;
    u32 bitMask = (1 << bit);
    bool support = capabilities[i].support;
    if (bit / 32 == 0) {
      if (deviceFeatures0 & bitMask) {
        if (support) {
          driverFeatures0 |= bitMask;
        } else {

        }
      }
      deviceFeatures0 &= ~bitMask;
    }
  }

  u32 driverFeatures1 = 0;
  for (u32 i = 0; i < capabilitiesLen; i += 1) {
    u32 bit = capabilities[i].bit;
    u32 bitMask = (1 << bit);
    bool support = capabilities[i].support;
    if (bit / 32 == 0) {
      if (deviceFeatures1 & bitMask) {
        if (support) {
          driverFeatures1 |= bitMask;
        } else {
          
        }
      }
      deviceFeatures1 &= ~bitMask;
    }
  }

  if (deviceFeatures0 || deviceFeatures1) {
    // device supports features that we (the driver) do not
  }

  WRITE32(regs->DriverFeaturesSel, 0);
  dsb(SY);
  WRITE32(regs->DriverFeatures, driverFeatures0);
  dsb(SY);
  WRITE32(regs->DriverFeaturesSel, 1);
  dsb(SY);
  WRITE32(regs->DriverFeatures, driverFeatures1);
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
