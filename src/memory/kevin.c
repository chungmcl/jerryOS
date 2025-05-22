// Kevin the Page Table Manager

#include "kevin.h"
#include "string.h"
#include "ppm.h"

bool setupPTM(const hardwareInfo* const hwInfo) {
  if (!ppmInit(
    hwInfo,
    1 // Pre-reserve exactly 1 page as the first page in memory already contains the stack and .bss
  )) return false;

  // Set the Intermediate Physical Address Size to 48 bits, 256TB
  // TCR_EL1.IPS = 0b101

  // Set Granule size for the TTBR0_EL1 to 16KB
  // TCR_EL1.TG0 = 0b10

  // Set Granule size for the TTBR1_EL1 to 16KB
  // TCR_EL1.TG1 = 0b01

  // If the Effective value of TCR_ELx.DS is 0, then the maximum VA and PA supported is 48 bits.
  // Set the maximum VA/PA to 48 bits
  // TCR_EL1.DS = 0

  // TCR_ELx.T0SZ configures the IA size of the lower VA range, translated using TTBR0_ELx.
  // TCR_ELx.T1SZ configures the IA size of the upper VA range, translated using TTBR1_ELx.

  // A translation table is required to be aligned to one of the following:
  //   • For the VMSAv8-64 translation system, if the translation table has fewer than eight entries and an OA size
  //   greater than 48 bits is used, then the table is aligned to 64 bytes.
  //   • Otherwise, the translation table is aligned to the size of that translation table.
  //   Only when all of the following are true is it possible to have fewer than 8 translation table entries:
  //   • The translation table is at the initial lookup level.
  //   • Concatenated translation tables are not used.
  //   RVCLZN If concatenated translation tables are used, then the concatenated translation tables are required to be aligned to the
  //   overall size of the memory occupied by the concatenated translation tables.

  // OA[51:48] are treated as 0b0000

  return true;
}