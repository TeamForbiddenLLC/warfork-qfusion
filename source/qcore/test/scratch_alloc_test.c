
#include "../qscratch_alloc.h"
#include "utest.h"

UTEST(alloc, scratchAlloc_1)
{
  struct QScratchAllocator testAlloc = {0};
  struct QScratchAllocDesc desc = {0};
  desc.blockSize = 1024;
  desc.alignment = 16;
  qInitScratchAllocator(&testAlloc, &desc);
  qScratchAlloc(&testAlloc, 128);
  qScratchAlloc(&testAlloc, 128);
  qScratchAlloc(&testAlloc, 128);
  qScratchAlloc(&testAlloc, 128);
  qScratchAlloc(&testAlloc, 128);
  qScratchAlloc(&testAlloc, 128);
  qScratchAlloc(&testAlloc, 128);
  qScratchAlloc(&testAlloc, 128);
  qFreeScratchAllocator(&testAlloc);
}


UTEST(alloc, scratchAlloc_2)
{
    struct QScratchAllocator testAlloc = { 0 };
    struct QScratchAllocDesc desc = { 0 };
    desc.blockSize = 1024;
    desc.alignment = 16;

    qInitScratchAllocator(&testAlloc, &desc);
    qScratchAlloc(&testAlloc, 128);
    qScratchAlloc(&testAlloc, 128);
    qScratchAlloc(&testAlloc, 128);
    void* data = qScratchAlloc(&testAlloc, 2048);
    EXPECT_EQ(((size_t)data) % 16, 0);
    qFreeScratchAllocator(&testAlloc);
}

UTEST_MAIN();
