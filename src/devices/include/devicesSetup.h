#include "jerryTypes.h"
#include "libfdt.h"

typedef struct {
  u64 deviceTreeLenBytes;
  uintptr ramStartPhysAddr;
  u64 ramLenBytes;
} deviceSetupInfo;

bool setupDevices(const void* deviceTreeAddress, deviceSetupInfo*const out);
