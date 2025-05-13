// Kevin the Page Table Manager

#include "kevin.h"

bool setupPTM() {
  // Set the Intermediate Physical Address Size to 48 bits, 256TB
  // TCR_EL1.IPS = 0b101

  // Set Granule size for the TTBR0_EL1 to 16KB
  // TCR_EL1.TG0 = 0b10

  // Set Granule size for the TTBR1_EL1 to 16KB
  // TCR_EL1.TG1 = 0b01

  return true;
}