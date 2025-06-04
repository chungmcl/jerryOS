// Kevin the Page Table Manager

#include "kevin.h"
#include "ppm.h"

__attribute__((section(".kernelRootTables")))
tableDescriptorS1 kernelRootTable0[8];
__attribute__((section(".kernelRootTables")))
tableDescriptorS1 kernelRootTable1[8];

static inline bool 👢🔫KernelPageTable(const jerryMetaData* const osMetaData) {
  /*
   * Boot👢strap🔫 the kernel VA by identity mapping the initial
   * kernel page table, i.e. mapping PA == VA. This is necessary because
   * after we turn the MMU on in enableMMU(), the processor will start
   * using the MMU to translate all referenced memory addresses via the page tables
   * pointed to by $TTBR0_EL1 and $TTBR1_EL1. This will also include the addresses
   * in $PC and $SP, meaning that after enableMMU() finishes and the MMU is on, the
   * processor will look into the page tables for the translation of ($PC + 4). Therefore,
   * we need to map the kernel's physical pages to a VA that is equal to its physical page's
   * PA such that the MMU is able to translate the VA ($PC + 4) to the PA ($PC + 4).
   * We setup the page kernel page table here to do this identity mapping before we call
   * enableMMU().
  */

  // Kernel addy range 1: From the page of the beginning of the kernel binary to the page of the end of .text.
  uintptr startPage = MEM_PAGE_LEN * ((uintptr)osMetaData->kernelBinStartAddr / MEM_PAGE_LEN);
  uintptr endPage = MEM_PAGE_LEN * ((uintptr)osMetaData->kernelTextEnd / MEM_PAGE_LEN);
  u32 pagesToMap = ((endPage - startPage) / MEM_PAGE_LEN) + 1;
  for (u32 i = 0; i < pagesToMap; i += 1) {
    uintptr curAddy = startPage + (MEM_PAGE_LEN * i);
    if (kevinMapPageToVA(kernelRootTable0, (void*)curAddy, curAddy, false) != curAddy) {
      return false;
    }
  }

  // Kernel addy range 2: From the kernel stack's page to the page of the end of .bss.
  startPage = MEM_PAGE_LEN * ((uintptr)osMetaData->kernelStackPointerStart / MEM_PAGE_LEN);
  endPage = MEM_PAGE_LEN * ((uintptr)osMetaData->kernelBssEnd / MEM_PAGE_LEN);
  pagesToMap = ((endPage - startPage) / MEM_PAGE_LEN) + 1;
  for (u32 i = 0; i < pagesToMap; i += 1) {
    uintptr curAddy = startPage + (MEM_PAGE_LEN * i);
    if (kevinMapPageToVA(kernelRootTable0, (void*)curAddy, curAddy, false) != curAddy) {
      return false;
    }
  }

  // Note that the Kernel's addy ranges should not be in the "high" range of the VA, i.e.
  // the VA range mapped by $TTBR1_EL1, so we don't do anything to kernelRootTable1.

  return true;
}

static inline void enableMMU(tableDescriptorS1* ttbr0_el1, tableDescriptorS1* ttbr1_el1, regTCR_EL1 tcr_el1) {
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
    : "r"(ttbr0_el1),
      "r"(ttbr1_el1),
      "r"(tcr_el1)
    : "memory"
  );
}

bool kevinInit(const jerryMetaData* const osMetaData) {
  if (!ppmInit(
    osMetaData,
    1 // Pre-reserve exactly 1 page as the first page in RAM already contains kernelRootTable, the stack, and .bss
  )) return false;

  if (!👢🔫KernelPageTable(osMetaData)) {
    return false;
  }
  enableMMU(
    (tableDescriptorS1*)&kernelRootTable0,
    (tableDescriptorS1*)&kernelRootTable1,
    (regTCR_EL1) {
      .TG0  = 0b10,   // Set Granule size for TTBR0_EL1 to 16KB
      .TG1  = 0b01,   // Set Granule size for TTBR1_EL1 to 16KB
      .DS   = 0b0,    // "If the Effective value of TCR_ELx.DS is 0, then the maximum VA and PA supported is 48 bits."
      .T0SZ = 25,     // TTBR0_EL1 VA address size == pow(2, (64 - T0SZ)); jerryOS will use 8-entry root L1 table -> VA[38:0] -> T0SZ=(64-39)=25
      .T1SZ = 25      // TTBR1_EL1 VA address size == pow(2, (64 - T1SZ)); jerryOS will use 8-entry root L1 table -> VA[38:0] -> T0SZ=(64-39)=25
    }
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

void* kevinMapPageToVA(tableDescriptorS1* rootTable, void* pagePA, uintptr va, bool overwrite) {
  u32 l1Idx = GET_BITS(va, 38, 36);
  u32 l2Idx = GET_BITS(va, 35, 25);
  u32 l3Idx = GET_BITS(va, 24, 14);

  tableDescriptorS1* l2Table;
  pageDescriptorS1* l3Table;

  if (kernelRootTable0[l1Idx].validBit && kernelRootTable0[l1Idx].tableDescriptor) {
    l2Table = NLTAToPA(kernelRootTable0[l1Idx].nlta);
  } else {
    l2Table = ppmGetPage();
    kernelRootTable0[l1Idx] = (tableDescriptorS1) {
      .validBit = 0b1,
      .tableDescriptor = 0b1,
      .nlta = PAToNLTA(l2Table)
    };
  }

  if (l2Table[l2Idx].validBit && l2Table[l2Idx].tableDescriptor) {
    l3Table = NLTAToPA(l2Table[l2Idx].nlta);
  } else {
    l3Table = ppmGetPage();
    l2Table[l2Idx] = (tableDescriptorS1) {
      .validBit = 0b1,
      .tableDescriptor = 0b1,
      .nlta = PAToNLTA(l3Table)
    };
  }

  if (l3Table[l3Idx].validBit && (l3Table[l3Idx].descriptorType == 0b1) && !overwrite) {
    return OABToPA(l3Table[l3Idx].oab);
  } else {
    l3Table[l3Idx] = (pageDescriptorS1) {
      .validBit = 0b1,
      .descriptorType = 0b1,
      .oab = PAToOAB(pagePA),
      .af = 0b1
    };
    return pagePA;
  }
}
