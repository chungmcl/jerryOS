// Kevin the Page Table Manager

#include "kevin.h"
#include "string.h"
#include "ppm.h"

bool setupPTM(const hardwareInfo* const hwInfo) {
  if (!ppmInit(
    hwInfo,
    1 // Pre-reserve exactly 1 page as the first page in memory already contains the stack and .bss
  )) return false;

  asm volatile (
    "msr tcr_el1, %0\n"
    "isb"
    : 
    : "r" ((regTCR_EL1) {
      .IPS  = 0b101,  // Set the Intermediate Physical Address Size to 48 bits, 256TB
      .TG0  = 0b10,   // Set Granule size for TTBR0_EL1 to 16KB
      .TG1  = 0b01,   // Set Granule size for TTBR1_EL1 to 16KB
      .DS   = 0b0,    // "If the Effective value of TCR_ELx.DS is 0, then the maximum VA and PA supported is 48 bits."
      .T0SZ = 25,     // TTBR0_EL1 VA address size == pow(2, (64 - T0SZ)); jerryOS will use 8-entry root L1 table -> VA[38:0] -> T0SZ=(64-39)=25
      .T1SZ = 25      // TTBR1_EL1 VA address size == pow(2, (64 - T1SZ)); jerryOS will use 8-entry root L1 table -> VA[38:0] -> T0SZ=(64-39)=25
    })
    : "memory"
  );

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