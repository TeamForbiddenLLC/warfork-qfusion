#include "r_gpu_ring_buffer.h"
#include "../gameshared/q_arch.h"
#include "../gameshared/q_math.h"


void R_RingOffsetAllocate( struct r_ring_offset_alloc_s *alloc, size_t requestSize, size_t *offset )
{
	assert( offset );

	assert( requestSize <= alloc->maxSize );
	if( ALIGN( alloc->currentOffset + requestSize, alloc->bufferAlignment ) > alloc->maxSize ) {
		alloc->currentOffset = 0;
	}
	( *offset ) = alloc->currentOffset;
	alloc->currentOffset += ALIGN( alloc->currentOffset + requestSize, alloc->bufferAlignment );
}
