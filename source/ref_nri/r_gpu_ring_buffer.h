#ifndef R_GPU_RING_BUFFER_H
#define R_GPU_RING_BUFFER_H

#include "r_nri.h"

struct r_ring_offset_alloc_s {
  size_t currentOffset;
  size_t maxSize;
  size_t bufferAlignment;
};
void R_RingOffsetAllocate( struct r_ring_offset_alloc_s* alloc, size_t requestSize, size_t* offset);

#endif
