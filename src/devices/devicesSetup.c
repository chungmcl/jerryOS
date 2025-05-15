#include "devicesSetup.h"
#include "virtio.h"
#include "libfdt.h"

bool setupDevices(const void* deviceTreeAddress, deviceSetupInfo*const out) {
  if (fdt_check_header(deviceTreeAddress) != 0) return false;

  s32 currNodeOffset = 0;
  s32 depth = 0;
  const void* currNode = (const void*)deviceTreeAddress;

  u64 deviceTreeLenBytes = fdt_totalsize(deviceTreeAddress);
  uintptr ramStartPhysAddr;
  u64 ramLenBytes;

  do {
    currNodeOffset = fdt_next_node(currNode, currNodeOffset, &depth);
    if (currNodeOffset < 0) {
      break;
    }
    const char* currNodeName = fdt_get_name(currNode, currNodeOffset, NULL);

    /**
     * Handle varying types of devices detected in device tree here
     */ 
    {
      if (strStartsWith(currNodeName, "memory@40000000")) {
        s32 regLen;
        const fdt32_t* regPtr = fdt_getprop(
          deviceTreeAddress, 
          currNodeOffset, 
          "reg", 
          &regLen
        );
        
        // We have to load each 32-bit int in individually first
        // instead of directly casting *(regPtr+0) and *(regPtr+1) to u64
        // as there's no guarantee regPtr+0 and regPtr+2 are aligned to 
        // a u64 (multiple of 8). We also have to convert from libfdt's endianness
        // to the CPU's endianess, handled by fdt32_to_cpu().
        fdt32_t reg0 = fdt32_to_cpu(regPtr[0]);
        fdt32_t reg1 = fdt32_to_cpu(regPtr[1]);
        fdt32_t reg2 = fdt32_to_cpu(regPtr[2]);
        fdt32_t reg3 = fdt32_to_cpu(regPtr[3]);
        ramStartPhysAddr = (((u64)reg0) << 32) | reg1;
        ramLenBytes =      (((u64)reg2) << 32) | reg3;
      } else if (strStartsWith(currNodeName, "virtio_mmio")) {
        u64 output;
        if (setupVirtIODevice(deviceTreeAddress, currNodeOffset, &output) == -1) {
          // device was not able to be setup
        }
      }
    }
    /* * */

  } while (currNodeOffset >= 0);

  out->deviceTreeLenBytes = deviceTreeLenBytes;
  out->ramStartPhysAddr = ramStartPhysAddr;
  out->ramLenBytes = ramLenBytes;

  return true;
}
