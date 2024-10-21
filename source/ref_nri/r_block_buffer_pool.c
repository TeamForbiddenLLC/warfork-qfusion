#include "r_block_buffer_pool.h"
#include "r_local.h"
#include "stb_ds.h"

void InitBlockBufferPool( struct nri_backend_s *nri, struct block_buffer_pool_s *pool, const struct block_buffer_pool_desc_s *desc )
{
	memset( pool, 0, sizeof( struct block_buffer_pool_s ) );
	pool->blockSize = desc->blockSize;
	pool->alignmentReq = desc->alignmentReq;
	pool->usageBits = desc->usageBits;
	pool->structureStride = desc->structureStride;
}

static inline void __InitPoolBlock( struct nri_backend_s *nri, NriBufferUsageBits usageBits, size_t blockSize, size_t structureStride, struct block_buffer_s *block )
{
	NriBufferDesc bufferDesc = { .size = blockSize, .structureStride = structureStride, .usage = usageBits};
	NRI_ABORT_ON_FAILURE( nri->coreI.CreateBuffer( nri->device, &bufferDesc, &block->buffer ) )
	NriResourceGroupDesc resourceGroupDesc = { .buffers = &block->buffer, .bufferNum = 1, .memoryLocation = NriMemoryLocation_HOST_UPLOAD };
	assert( nri->helperI.CalculateAllocationNumber( nri->device, &resourceGroupDesc ) == 1 );
	NRI_ABORT_ON_FAILURE( nri->helperI.AllocateAndBindMemory( nri->device, &resourceGroupDesc, &block->memory ) );
}

struct block_buffer_pool_req_s BlockBufferPoolReq( struct nri_backend_s *nri, struct block_buffer_pool_s *pool, size_t reqSize )
{
	const size_t alignReqSize = ALIGN( reqSize, pool->alignmentReq );
	if( !pool->current.buffer ) {
		__InitPoolBlock( nri, pool->usageBits, pool->blockSize, pool->structureStride, &pool->current );
		pool->blockOffset = 0;
	} else if( pool->blockOffset + alignReqSize > pool->blockSize ) {
		arrpush( pool->recycle, pool->current );
		const size_t poolSize = arrlen( pool->pool );
		if( poolSize > 0 ) {
			memcpy( &( pool->current ), &( pool->pool[poolSize - 1] ), sizeof( struct block_buffer_s ) );
			arrsetlen( pool->pool, poolSize - 1 );
		} else {
			__InitPoolBlock( nri, pool->usageBits, pool->blockSize, pool->structureStride, &pool->current );
			// NriBufferDesc bufferDesc = { .size = pool->blockSize, .structureStride = pool->structureStride };
			// NRI_ABORT_ON_FAILURE( nri->coreI.CreateBuffer( nri->device, &bufferDesc, &pool->current.buffer ) )
			// NriResourceGroupDesc resourceGroupDesc = { .buffers = &pool->current.buffer, .bufferNum = 1, .memoryLocation = NriMemoryLocation_HOST_UPLOAD };
			// assert( nri->helperI.CalculateAllocationNumber( nri->device, &resourceGroupDesc ) == 1 );
			// NRI_ABORT_ON_FAILURE( nri->helperI.AllocateAndBindMemory( nri->device, &resourceGroupDesc, &pool->current.memory ) );
			// //arrput( pool->recycle, pool->current );
			// pool->current.cpuMapped = nri->coreI.MapBuffer( pool->current.buffer, 0, NRI_WHOLE_SIZE );
		}
		pool->blockOffset = 0;
	}

	struct block_buffer_pool_req_s req = { 0 };
	req.buffer = pool->current.buffer;
	req.bufferOffset = pool->blockOffset;
	req.bufferSize = reqSize;
	pool->blockOffset += alignReqSize;
	return req;
}

struct nri_descriptor_s BlockReqToDescriptorWrapper( struct nri_backend_s *nri, struct block_buffer_pool_req_s *req, NriBufferViewType viewType )
{
	NriDescriptor *descriptor = 0;
	NriBufferViewDesc bufferDesc = { .buffer = req->buffer, .size = req->bufferSize, .offset = req->bufferOffset, .format = viewType };
	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateBufferView( &bufferDesc, &descriptor ) );
	return R_CreateDescriptorWrapper( &rsh.nri, descriptor );
}

void BlockBufferPoolReset( struct block_buffer_pool_s *pool )
{
	for( size_t i = 0; i < arrlen( pool->recycle ); i++ ) {
		arrpush( pool->pool, pool->recycle[i] );
	}
	pool->blockOffset = 0;
	arrsetlen( pool->recycle, 0 );
}

void BlockBufferPoolFree( struct nri_backend_s *nri, struct block_buffer_pool_s *pool )
{
	for( size_t i = 0; i < arrlen( pool->recycle ); i++ ) {
		nri->coreI.DestroyBuffer( pool->recycle[i].buffer );
		nri->coreI.FreeMemory( pool->recycle[i].memory );
	}

	for( size_t i = 0; i < arrlen( pool->pool ); i++ ) {
		nri->coreI.DestroyBuffer( pool->pool[i].buffer );
		nri->coreI.FreeMemory( pool->pool[i].memory );
	}
	arrfree( pool->recycle );
	arrfree( pool->pool );
}
