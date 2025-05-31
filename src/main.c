#include "main.h"
#include "devicesSetup.h"
#include "kevin.h"

s32 main() {
  // note that the device tree is in big endian while this CPU is lil endian
  extern u8 _kernel_bin;
  extern u8 _rodata_start;
  extern u8 _rodata_end;
  extern u8 _text_start;
  extern u8 _text_end;
  extern u8 _bss_start;
  extern u8 _bss_end;
  const void* deviceTreeAddress;
  const void* stackPointerStart;
  asm volatile (
    "mov %0, x1\n" // Get the device tree's address out of x1
    "mov %1, x2" // Get the kernel initial stack pointer address address out of x2
    : "=r"(deviceTreeAddress), 
      "=r"(stackPointerStart)
    : // No input operands
    : // No clobbered registers
  );
  
  jerryMetaData osMetaData = {
    .kernelBinStartAddr      = (void*)&_kernel_bin,
    .kernelRodataStart       = (void*)&_rodata_start,
    .kernelRodataEnd         = (void*)&_rodata_end,
    .kernelTextStart         = (void*)&_text_start,
    .kernelTextEnd           = (void*)&_text_end,
    .kernelStackPointerStart =          stackPointerStart,
    .kernelBssStart          = (void*)&_bss_start,
    .kernelBssEnd            = (void*)&_bss_end,
  };
  if (!setupDevices(deviceTreeAddress, &osMetaData)) {
    // panic
  }

  u64 memSizeGB = osMetaData.ramLen >> 30;

  if (!kevinInit(&osMetaData)) {
    // panic
  }

  return 0;
}
