// Kevin the Page Table Manager

#include "jerryTypes.h"

#define MEM_PAGE_LEN (0b1 << 14) // 16KB

bool setupPTM(const hardwareInfo*const hwInfo);