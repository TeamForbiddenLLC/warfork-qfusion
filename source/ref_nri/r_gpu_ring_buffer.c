#include "r_gpu_ring_buffer.h"
#include "../gameshared/q_arch.h"
#include "../gameshared/q_math.h"

void R_InitRingBuffer( struct nri_backend_s *nri, struct ring_buffer_desc_s *desc, struct r_ring_buffer_s *ring )
{
	assert( nri );
	assert( ring );
  
  ring->bufferAlignment = desc->alignment;

	NriBufferDesc bufferDesc = { .size = desc->size};
	NRI_ABORT_ON_FAILURE( nri->coreI.CreateBuffer( nri->device, &bufferDesc, &ring->buffer ) )

	NriResourceGroupDesc resourceGroupDesc = { .buffers = &ring->buffer, .bufferNum = 1, .memoryLocation = desc->memoryLocation };
	ring->numMemoryLen =  nri->helperI.CalculateAllocationNumber( nri->device, &resourceGroupDesc );
	assert(ring->numMemoryLen < Q_ARRAY_COUNT(ring->memory));
	NRI_ABORT_ON_FAILURE( nri->helperI.AllocateAndBindMemory( nri->device, &resourceGroupDesc, ring->memory) );

}

void R_RequestBuffer( struct r_ring_buffer_s *ring, size_t requestSize, struct buffer_req_s* req) {
  assert(requestSize <= ring->maxSize);
	if( ALIGN( ring->currentOffset + requestSize, ring->bufferAlignment ) > ring->maxSize ) {
		ring->currentOffset = 0;
	}
	req->offset = ring->currentOffset;
	req->buffer = ring->buffer;
	ring->currentOffset += ALIGN( ring->currentOffset + requestSize, ring->bufferAlignment );
}

void R_FreeRingBuffer( struct nri_backend_s *nri,struct r_ring_buffer_s *ring ) {
	nri->coreI.DestroyBuffer(ring->buffer);
	for( size_t i = 0; i < ring->numMemoryLen; i++ ) {
		nri->coreI.FreeMemory( ring->memory[i] );
	}
}
