// Kevin the Page Table Manager

#include "kevin.h"
#include "string.h"
#include "ppm.h"
#include "translationTableDescriptors.h"

__attribute__((section(".kernelRootTable")))
tableDescriptorS1 kernelRootTable[8];

__attribute__((section(".text")))
uintptr kernelText;

bool setupPTM(const hardwareInfo* const hwInfo) {
  if (!ppmInit(
    hwInfo,
    1 // Pre-reserve exactly 1 page as the first page in RAM already contains kernelRootTable, the stack, and .bss
  )) return false;

  // Map the kernel into TTBR1 space
  kernelRootTable[7] = (tableDescriptorS1) {
    .validBit = 0b1,
    .tableDescriptor = 0b1,
    .nlta = (uintptr)ppmGetPage()
  };
  tableDescriptorS1* l2table = (tableDescriptorS1*)kernelRootTable[7].nlta;
  l2table[0] = (tableDescriptorS1) {
    .validBit = 0b1,
    .tableDescriptor = 0b1,
    .nlta = (uintptr)ppmGetPage()
  };
  pageDescriptorS1* l3table = (pageDescriptorS1*)l2table[0].nlta;
  for (u32 i = 0; i < (MEM_PAGE_LEN / sizeof(pageDescriptorS1)); i += 1) {
    l3table[i] = (pageDescriptorS1) {
      .validBit = 0b1,
      .descriptorType = 0b1,
      .oab = ((uintptr)hwInfo->kernelBinStartAddr + (MEM_PAGE_LEN * i))
    };
  }

  // Map
  // • The first page in memory (contains kernelRootTable, kernel stack, and .bss)
  // • The hwPageFreeList's pages
  // • kernelRootTable[7]'s branch's pages
  // into TTBR1 space

  asm volatile (
    "msr ttbr0_el1, %0\n" // Set TTBR0.
    "msr ttbr1_el1, %1\n" // Set TTBR1.
    "msr tcr_el1, %2\n"   // Set TCR.
    "isb\n"               // The ISB forces these changes to be seen before the MMU is enabled.
    "mrs %0, sctlr_el1\n" // Read System Control Register configuration data. (Reuse register %0; no more correlation to input param 1's value).
    "orr %0, %0, #1\n"    // Set [M] bit and enable the MMU.
    "msr sctlr_el1, %0\n" // Write System Control Register configuration data.
    "isb"                 // The ISB forces these changes to be seen by the next instruction.
    : // No output operands
    : "r"(0xffffffffffffffff),
      "r"(&kernelRootTable),
      "r"((regTCR_EL1) {
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