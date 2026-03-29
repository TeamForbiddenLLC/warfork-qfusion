#ifndef Q_SCRATCH_ALLOCATOR_H_INCLUDED
#define Q_SCRATCH_ALLOCATOR_H_INCLUDED

#include "qarch.h"
#include "qtypes.h"

struct QScratchAllocator {
	struct QAllocScratchBlock *freeBlocks;
	struct QAllocScratchBlock *current;
	size_t pos;
	size_t blockSize;
	uint16_t alignment;
};

struct QScratchAllocDesc {
	size_t blockSize;
	size_t alignment;
};

#ifdef __cplusplus
extern "C" {
#endif

void qInitScratchAllocator( struct QScratchAllocator *alloc, struct QScratchAllocDesc *desc );
void qFreeScratchAllocator(struct QScratchAllocator* alloc);
void qResetScratchAllocator(struct QScratchAllocator* alloc);
void *qScratchAlloc( struct QScratchAllocator *alloc, size_t size );

#ifdef __cplusplus
}
#endif

#endif
