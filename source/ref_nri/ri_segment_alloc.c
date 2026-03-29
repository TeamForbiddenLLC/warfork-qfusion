#include "ri_segment_alloc.h"

void InitRISegmentAlloc( struct RISegmentAlloc_s *pool, const struct RISegmentAllocDesc_s *desc )
{
	assert( pool );
	assert(pool->numSegments < MAX_NUMBER_SEGMENTS);
	memset( pool, 0, sizeof( struct RISegmentAlloc_s ) );

	assert( desc );
	pool->elementStride = desc->elementStride;
	pool->tail = 0;
	pool->head = 1;
	pool->numSegments = desc->numSegments;
	pool->maxElements = desc->maxElements;
	assert( pool->elementStride > 0 );
}

//void FreeRISegmentAlloc( struct RIDevice_s *device, struct RISegmentAlloc_s *pool )
//{
//	GPU_VULKAN_BLOCK( device->renderer, ( {
//						  if( pool->vk.allocator )
//							  vmaFreeMemory( device->vk.vmaAllocator, pool->vk.allocator );
//						  if( pool->vk.blockView )
//							  vkDestroyBufferView( device->vk.device, pool->vk.blockView, NULL );
//						  if( pool->vk.buffer )
//							  vkDestroyBuffer( device->vk.device, pool->vk.buffer, NULL );
//					  } ) );
//}

bool RISegmentAlloc(  uint32_t frameIndex, struct RISegmentAlloc_s *alloc, size_t numElements, struct RISegmentReq_s *req )
{
	//size_t maxBufferElements = alloc->maxElements;

	// reclaim segments that are unused
	while( alloc->tail != alloc->head && frameIndex >= ( alloc->segment[alloc->tail].frameNum + alloc->numSegments ) ) {
		assert( alloc->numElements >= alloc->segment[alloc->tail].numElements );
		alloc->numElements -= alloc->segment[alloc->tail].numElements;
		alloc->elementOffset = ( alloc->elementOffset + alloc->segment[alloc->tail].numElements ) % alloc->maxElements;
		alloc->segment[alloc->tail].numElements = 0;
		alloc->segment[alloc->tail].frameNum = 0;
		alloc->tail = ( alloc->tail + 1 ) % Q_ARRAY_COUNT( alloc->segment );
	}

	// the frame has change
	if( frameIndex  != alloc->segment[alloc->head].frameNum ) {
		alloc->head = ( alloc->head + 1 ) % Q_ARRAY_COUNT( alloc->segment );
		alloc->segment[alloc->head].frameNum = frameIndex ;
		alloc->segment[alloc->head].numElements = 0;
		assert( alloc->head != alloc->tail ); // this shouldn't happen
	}

	size_t elmentEndOffset = ( alloc->elementOffset + alloc->numElements ) % alloc->maxElements;
	assert( alloc->elementOffset < alloc->maxElements );
	assert( elmentEndOffset < alloc->maxElements);
	// we don't have enough space to fit into the end of the buffer give up the remaning and move the cursor to the start
	if( alloc->elementOffset < elmentEndOffset && elmentEndOffset + numElements > alloc->maxElements ) {
		const uint32_t remaining = ( alloc->maxElements - elmentEndOffset );
		alloc->segment[alloc->head].numElements += remaining;
		alloc->numElements += remaining;
		elmentEndOffset = 0;
		assert( ( alloc->elementOffset + alloc->numElements ) % alloc->maxElements == 0 );
	}
	size_t remainingSpace = 0;
	if( elmentEndOffset < alloc->elementOffset ) { // the buffer has wrapped around
		remainingSpace = alloc->elementOffset - elmentEndOffset;
	} else {
		remainingSpace = alloc->maxElements - elmentEndOffset;
	}
	assert( remainingSpace <= alloc->maxElements );

	// there is not enough avalaible space we need to reallocate
	if( numElements > remainingSpace ) {
	  return false;

		////// attach it to the cmd buffer to be cleaned up on the round trip
		//// arrpush( cmd->freeBuffers, alloc->buffer );
		//// arrpush( cmd->freeMemory, alloc->memory );
		//do {
		//	maxBufferElements = ( maxBufferElements + ( maxBufferElements >> 1 ) );
		//} while( maxBufferElements < numElements );
		//for( size_t i = 0; i < Q_ARRAY_COUNT( alloc->segment ); i++ ) {
		//	alloc->segment[i].numElements = 0;
		//	alloc->segment[i].frameNum = 0;
		//}
		//alloc->numElements = 0;
		//alloc->elementOffset = 0;
		//elmentEndOffset = 0; // move the cursor to the beginning
		//
		//alloc->bufferSize = alloc->reallocFunc( device, alloc, maxBufferElements);

		// NriBufferDesc initBufferDesc = {
		//	.size =  maxBufferElements * alloc->elementStride,
		//	.structureStride = alloc->elementStride,
		//	.usage = alloc->usageBits
		// };

		// NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateBuffer( rsh.nri.device, &initBufferDesc, &alloc->buffer ) )
		// NriResourceGroupDesc resourceGroupDesc = { .buffers = &alloc->buffer, .bufferNum = 1, .memoryLocation = NriMemoryLocation_HOST_UPLOAD };
		// assert( rsh.nri.helperI.CalculateAllocationNumber( rsh.nri.device, &resourceGroupDesc ) == 1 );
		// NRI_ABORT_ON_FAILURE( rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, &alloc->memory ) );
	}
	alloc->segment[alloc->head].numElements += numElements;
	alloc->numElements += numElements;

	req->elementOffset = elmentEndOffset;
	req->elementStride = alloc->elementStride;
	req->numElements = numElements; // includes the padding on the end of the buffer
  return true;
}
