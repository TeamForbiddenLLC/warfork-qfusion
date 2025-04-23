
#include "../qscratch_alloc.h"
#include "utest.h"
#define MEM_DEFINE_INTERFACE_IMPL_SYSTEM 1
#include "../../qcommon/mod_mem.h"

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
