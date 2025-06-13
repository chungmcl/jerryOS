#include "jerryOS.h"

bool ppmInit(const jerryMetaData* const osMetaData, u32 numPreReservedPages);
void* ppmGetPage();
bool ppmFreePage(void* page, bool clean);