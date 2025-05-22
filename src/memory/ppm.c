#include "ppm.h"
#include "string.h"

static uintptr ramAddy;
static u32 numPhysPages;
/*
* The HW page free list will be an array of u8s,
* where each bit represents free state of a single HW page
*/
static u8* hwPageFreeList;
// NOTE: hwPageFreeListLen == # of bytes; != # of bits
static u32 hwPageFreeListLen;

static        bool markPageRange(u32 upperIdx, u32 lowerIdx, bool used);
static inline bool markPageRangeUsed(u32 upperIdx, u32 lowerIdx) { return markPageRange(upperIdx, lowerIdx, true); }
static inline bool markPageRangeFree(u32 upperIdx, u32 lowerIdx) { return markPageRange(upperIdx, lowerIdx, false); }

bool ppmInit(const hardwareInfo* const hwInfo, u32 numPreReservedPages) {
  ramAddy = hwInfo->ramStartAddr;
  numPhysPages = hwInfo->ramLen / MEM_PAGE_LEN;
  hwPageFreeListLen = numPhysPages / 8;

  u32 numAlreadyUsedPages = numPreReservedPages;
  // Preemptively reserve pages for the hwPageFreeList itself
  numAlreadyUsedPages += (hwPageFreeListLen / MEM_PAGE_LEN);

  // Place the hwPageFreeList starting from the second page in memory,
  // after the stack and .bss
  hwPageFreeList = (u8*)(ramAddy + MEM_PAGE_LEN);

  return markPageRangeUsed(
    numAlreadyUsedPages - 1, // -1 cause markPageRangeUsed() takes page INDICES as parameters
    0
  );
}

static bool markPageRange(u32 upperIdx, u32 lowerIdx, bool used) {
  if (upperIdx < lowerIdx) return false;
  u32 upperByteIdx = upperIdx / 8;
  u32 lowerByteIdx = lowerIdx / 8;

  if (upperByteIdx == lowerByteIdx) {
    u8* byte = hwPageFreeList + lowerByteIdx;
    u8 mask = ((0b1 << (upperIdx - lowerIdx + 1)) - 1) << (lowerIdx % 8);
    *byte = used ? (*byte | mask) : (*byte & ~mask);
    return true;
  }

  if ((upperIdx + 1) % 8 != 0) {
    u8* byte = hwPageFreeList + upperByteIdx;
    u8 mask = (0b1 << ((upperIdx % 8) + 1)) - 1;
    *byte = used ? (*byte | mask) : (*byte & ~mask);
    upperByteIdx -= 1;
  } if ((lowerIdx % 8) != 0) {
    u8* byte = hwPageFreeList + lowerByteIdx;
    u8 mask = (0b1 << ((lowerIdx % 8))) - 1;
    *byte = used ? (*byte | ~mask) : (*byte & mask);
    lowerByteIdx += 1;
  }
  memset(hwPageFreeList + lowerByteIdx, used ? 0xFF : 0x00, upperByteIdx - lowerByteIdx + 1);
  return true;
}