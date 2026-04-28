#include "ri_segment_alloc.h"

void InitRISegmentAlloc( struct RISegmentAlloc_s *pool, const struct RISegmentAllocDesc_s *desc )
{
	assert( pool );
	assert( desc );
	assert( desc->numSegments < MAX_NUMBER_SEGMENTS );
	memset( pool, 0, sizeof( struct RISegmentAlloc_s ) );

	pool->elementStride = desc->elementStride;
	pool->elementsConsumed = 0;
	pool->tail = 0;
	pool->head = 1;
	pool->numSegments = desc->numSegments;
	pool->maxElements = desc->maxElements;
	assert( pool->elementStride > 0 );
}

bool RISegmentAlloc( uint32_t frameIndex, struct RISegmentAlloc_s *alloc, size_t numElements, struct RISegmentReq_s *req )
{
	// reclaim segments that are unused
	while( alloc->tail != alloc->head && frameIndex >= ( alloc->segment[alloc->tail].frameNum + alloc->numSegments ) ) {
		assert( alloc->elementsConsumed >= alloc->segment[alloc->tail].numElements );
		alloc->elementsConsumed -= alloc->segment[alloc->tail].numElements;
		alloc->segment[alloc->tail].numElements = 0;
		alloc->segment[alloc->tail].frameNum = 0;
		alloc->tail = ( alloc->tail + 1 ) % Q_ARRAY_COUNT( alloc->segment );
	}

	// the frame has changed
	if( frameIndex != alloc->segment[alloc->head].frameNum ) {
		alloc->head = ( alloc->head + 1 ) % Q_ARRAY_COUNT( alloc->segment );
		alloc->segment[alloc->head].frameNum = frameIndex;
		alloc->segment[alloc->head].numElements = 0;
		assert( alloc->head != alloc->tail ); // this shouldn't happen
	}

	assert( alloc->elementOffset < alloc->maxElements );

	// not enough total free space
	if( ( alloc->maxElements - alloc->elementsConsumed ) < numElements ) {
		return false;
	}

	// try to fit linearly from the current write cursor
	if( alloc->elementOffset + numElements <= alloc->maxElements ) {
		req->elementOffset = alloc->elementOffset;
		req->elementStride = alloc->elementStride;
		req->numElements = numElements;

		alloc->elementOffset += numElements;
		alloc->elementsConsumed += numElements;
		alloc->segment[alloc->head].numElements += numElements;
		return true;
	}

	// doesn't fit at the end — wrap around to offset 0
	// account for the padding we skip at the tail of the buffer
	const size_t elementTailRemainder = alloc->maxElements - alloc->elementOffset;
	if( ( alloc->maxElements - ( alloc->elementsConsumed + elementTailRemainder ) ) >= numElements ) {
		// charge the skipped tail padding to the current segment so reclaim accounts for it
		alloc->segment[alloc->head].numElements += elementTailRemainder;
		alloc->elementsConsumed += elementTailRemainder;

		req->elementOffset = 0;
		req->elementStride = alloc->elementStride;
		req->numElements = numElements;

		alloc->elementOffset = numElements;
		alloc->elementsConsumed += numElements;
		alloc->segment[alloc->head].numElements += numElements;
		return true;
	}

	return false;
}
