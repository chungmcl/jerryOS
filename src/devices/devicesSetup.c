#include "devicesSetup.h"
#include "virtio.h"

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

    /**
     * Handle varying types of devices detected in device tree here
     */ 
    {
      if (strStartsWith(currNodeName, "virtio_mmio")) {
        if (!setupVirtIODevice(deviceTreeAddress, currNodeOffset)) return false;
      }
    }
    /* * */

  } while (currNodeOffset >= 0);

  return true;
}
