#include "virtio.h"
#include "virtioBlk.h"

u64 negotiateFeatures(
  // inputs
  virtioRegs* deviceRegisters,
  u32 interruptID,
  u64 driverFeatures,
  // outputs
  u64* unsupportedDriverFeatures,
  u64* unsupportedDeviceFeatures
) {
  u64 deviceFeatures = READ32(deviceRegisters->DeviceFeatures);
  dsb(SY);
  WRITE32(deviceRegisters->DeviceFeaturesSel, 1);
  dsb(SY);
  deviceFeatures |= ((u64)READ32(deviceRegisters->DeviceFeatures)) << 32;

  *unsupportedDriverFeatures = driverFeatures & ~deviceFeatures;
  *unsupportedDeviceFeatures = deviceFeatures & ~driverFeatures;
  return driverFeatures & deviceFeatures;
}

bool setupBlockDevice(
  // inputs
  virtioRegs* blockDeviceRegs, u32 interruptID,
  // output
  u64* deviceSizeBytes
) {
  // See virtio spec section "2.3 Device Configuration Space"
  // for an explanation of the ConfigGeneration register as used below
  u32 before, after;
  do {
    before = READ32(blockDeviceRegs->ConfigGeneration);
    //
    volatile virtioBlkConfig* config = (virtioBlkConfig*)blockDeviceRegs->Config;
    *deviceSizeBytes = ((u64)READ32(config->capacity)) * VIRTIO_BLK_BLOCK_SIZE;
    //
    after = READ32(blockDeviceRegs->ConfigGeneration);
  } while (after != before);

  // jerry is lazy and will support every feature the device does
  u64 driverFeatures = 0xFFFFFFFFFFFFFFFFUL;

  // 4. Read device feature bits, and write the subset of feature bits understood by the OS and driver to the
  // device. During this step the driver MAY read (but MUST NOT write) the device-specific configuration
  // fields to check that it can support the device before accepting it.
  u64 unsupportedDriverFeatures;
  u64 unsupportedDeviceFeatures;
  u64 matchedFeatures = negotiateFeatures(
    blockDeviceRegs, interruptID, 
    driverFeatures,
    &unsupportedDriverFeatures,
    &unsupportedDeviceFeatures
  );

  u32 driverFeatures0 = (u32)(matchedFeatures);
  u32 driverFeatures1 = (u32)(matchedFeatures >> 32);

  dsb(SY);
  WRITE32(blockDeviceRegs->DriverFeaturesSel, 0);
  dsb(SY);
  WRITE32(blockDeviceRegs->DriverFeatures, driverFeatures0);
  dsb(SY);
  WRITE32(blockDeviceRegs->DriverFeaturesSel, 1);
  dsb(SY);
  WRITE32(blockDeviceRegs->DriverFeatures, driverFeatures1);
  dsb(SY);

  // 5. Set the FEATURES_OK status bit. The driver MUST NOT accept new feature bits after this step.
  WRITE32(blockDeviceRegs->Status, READ32(blockDeviceRegs->Status) | VIRTIO_STATUS_FEATURES_OK);
	dsb(SY);

  // 6. Re-read device status to ensure the FEATURES_OK bit is still set: otherwise, the device does not
  // support our subset of features and the device is unusable.
	if (!(READ32(blockDeviceRegs->Status) & VIRTIO_STATUS_FEATURES_OK)) {
		return false;
	}

  // TODO(chungmcl):
  // 7. Perform device-specific setup, including discovery of virtqueues for the device, optional per-bus setup,
  // reading and possibly writing the device’s virtio configuration space, and population of virtqueues.

  // 8. Set the DRIVER_OK status bit. At this point the device is “live”.
  WRITE32(blockDeviceRegs->Status, READ32(blockDeviceRegs->Status) | VIRTIO_STATUS_DRIVER_OK);
	dsb(SY);

  return true;
}

i32 setupVirtIODevice(const void* deviceTreeAddress, i32 virtIODevNodeOffset, void* output) {
  // If any of these steps go irrecoverably wrong, the driver SHOULD set the FAILED status bit to indicate that it
  // has given up on the device (it can reset the device later to restart if desired). The driver MUST NOT continue
  // initialization in that case.
  // The driver MUST NOT notify the device before setting DRIVER_OK.
  i32 regLen;
  const fdt32_t* regPtr = fdt_getprop(deviceTreeAddress, virtIODevNodeOffset, "reg", &regLen);
  virtioRegs* regs = (virtioRegs*)((u64)fdt32_to_cpu(regPtr[1]));

  i32 interruptsLen;
  const fdt32_t* interruptsPtr = fdt_getprop(deviceTreeAddress, virtIODevNodeOffset, "interrupts", &interruptsLen);
  u32 interruptID = (u32)fdt32_to_cpu(interruptsPtr[1]) + 32; // Idk why we have to add a constant 32...?

  u32 magicValue = READ32(regs->MagicValue);
  if (READ32(regs->MagicValue) != VIRTIO_MAGIC) {
    // Wrong magic value
    return -1;
  }
  u32 version = READ32(regs->Version);
  if (READ32(regs->Version) != VIRTIO_VERSION) {
    // Wrong VirtIO version
    return -1;
  }

  // 1. Reset the device.
  WRITE32(regs->Status, 0);
  dsb(SY);

  // 2. Set the ACKNOWLEDGE status bit: the guest OS has notice the device.
  WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_ACKNOWLEDGE);
  dsb(SY);

  // 3. Set the DRIVER status bit: the guest OS knows how to drive the device.
  WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_DRIVER);
  dsb(SY);
      
  u32 deviceID = READ32(regs->DeviceID);
  switch (deviceID) {
    case VIRTIO_DEV_BLK: {
      u64 blockDeviceSizeBytes;
      if (!setupBlockDevice(regs, interruptID, &blockDeviceSizeBytes)) {
        return -1;
      }
      *((u64*)output) = blockDeviceSizeBytes;
    }
    default: {
      // Unsupported VirtIO device
      return -1;
    }
  }

  return (i32)deviceID;
}
