#ifndef R_GPU_RING_BUFFER_H
#define R_GPU_RING_BUFFER_H

#include "r_nri.h"

struct r_ring_buffer_s {
  size_t numMemoryLen;
  NriMemory* memory[4];
  NriBuffer* buffer;
  size_t currentOffset;
  size_t maxSize;
  size_t bufferAlignment;
};

struct ring_buffer_desc_s {
  NriMemoryLocation memoryLocation;

  size_t size; // the size of the buffer
	size_t alignment;
  size_t stride;
};

void R_InitRingBuffer( struct nri_backend_s *nri, struct ring_buffer_desc_s *desc, struct r_ring_buffer_s *buffer );
void R_FreeRingBuffer( struct nri_backend_s *nri,struct r_ring_buffer_s *buffer );

struct ringe_buffer_req_s {
  NriBuffer* buffer;
  size_t bufferOffset;
};

struct buffer_req_s {
  size_t offset;
  NriBuffer* buffer;
};
void R_RequestBuffer(struct r_ring_buffer_s* buffer , size_t requestSize, struct buffer_req_s* req);

#endif
