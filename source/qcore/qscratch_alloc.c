
#include "qscratch_alloc.h"

struct QAllocScratchBlock {
	struct QAllocScratchBlock *next;
	size_t len;
	void *data[];
};
static size_t __scratchDefaultBlockLen( struct QScratchAllocator *alloc )
{
	return alloc->blockSize + alloc->alignment;
}

void qInitScratchAllocator( struct QScratchAllocator *alloc, struct QScratchAllocDesc *desc )
{
	assert( alloc );
	assert( desc );
	memset( alloc, 0, sizeof( struct QScratchAllocator ) );
	alloc->blockSize = desc->blockSize;
	alloc->alignment = Q_MAX( desc->alignment, sizeof( uint16_t ) );
}

void qResetScratchAllocator( struct QScratchAllocator *alloc )
{
	alloc->freeBlocks = NULL;
	struct QAllocScratchBlock *blk = alloc->freeBlocks;
	while( blk ) {
		struct QAllocScratchBlock *current = blk; // we only reset the scractch allocator to the current block which is the default blockSize
		blk = blk->next;
		if( current == alloc->current ) {
			current->next = NULL;
			alloc->freeBlocks = current;
			continue;
		}
		free( current );
	}
	alloc->pos = 0;
}

void qFreeScratchAllocator( struct QScratchAllocator *alloc )
{
	struct QAllocScratchBlock *blk = alloc->freeBlocks;
	while( blk ) {
		struct QAllocScratchBlock *current = blk;
		blk = blk->next;
		free( current );
	}
	alloc->freeBlocks = NULL;
	alloc->current = NULL;
}

void *qScratchAlloc( struct QScratchAllocator *alloc, size_t size )
{
	const size_t reqSize = Q_ALIGN_TO( size, alloc->alignment );
	if( reqSize > alloc->blockSize ) {
		const size_t len = size + alloc->alignment; 
		struct QAllocScratchBlock *block = (struct QAllocScratchBlock *)malloc( sizeof( struct QAllocScratchBlock ) + len );
		block->len = len;
		
		block->next = alloc->freeBlocks;
		alloc->freeBlocks = block;
		
		const size_t offset = ( (size_t)block->data ) % alloc->alignment;
		return ( (uint8_t *)block->data ) + offset;
	}

  if( alloc->current == NULL || ( alloc->pos + reqSize ) > alloc->current->len ) {
		
		const size_t defaultScratchLen = __scratchDefaultBlockLen(alloc); 
		struct QAllocScratchBlock *block = (struct QAllocScratchBlock *)malloc( sizeof( struct QAllocScratchBlock ) + defaultScratchLen );
		block->len = defaultScratchLen;

		alloc->pos = ( (size_t)block->data ) % alloc->alignment;

		block->next = alloc->freeBlocks;
		alloc->freeBlocks = block;
		alloc->current = block;
	}
	void *result = ( (uint8_t *)alloc->current->data ) + alloc->pos;
	alloc->pos += reqSize;
	return result;
}
