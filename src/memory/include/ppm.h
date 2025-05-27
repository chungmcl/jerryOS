#include "jerryOS.h"

bool ppmInit(const hardwareInfo* const hwInfo, u32 numPreReservedPages);
void* ppmGetPage();
bool ppmFreePage(void* page, bool clean);