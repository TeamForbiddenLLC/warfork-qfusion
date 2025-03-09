#ifndef RI_RINAGE_ALLOC_H
#define RI_RINAGE_ALLOC_H
#include "../../gameshared/q_arch.h"
#include "ri_types.h"

#define MAX_NUMBER_SEGMENTS 8
#include "qtypes.h"

struct RISegmentAlloc_s {
  // configuration
//  RISegmentRealloc_Func reallocFunc; 
 	uint16_t elementStride;
 	uint16_t numSegments;
  uint16_t maxElements;

  // data
  int16_t tail;
  int16_t head;
  uint16_t numElements;
  size_t elementOffset;
  struct {
    uint64_t frameNum;
    size_t numElements; // size of the generation
  } segment[MAX_NUMBER_SEGMENTS]; // allocations are managed in segments
};

struct RISegmentReq_s {
	uint16_t elementStride;
	uint32_t elementOffset;
	uint32_t numElements;
};

struct RISegmentAllocDesc_s{
	uint32_t numElements;
	uint16_t elementStride;
	uint16_t numSegments;
	uint16_t maxElements; 
//	RISegmentRealloc_Func alloc;
};

void InitRISegmentAlloc( struct RISegmentAlloc_s *pool, const struct RISegmentAllocDesc_s *desc );
bool RISegmentAlloc( uint32_t frameIndex, struct RISegmentAlloc_s *alloc, size_t numElements, struct RISegmentReq_s *req );

static inline bool IsRISegmentBufferContinous( size_t currentOffset, size_t currentNumElements, size_t nextOffset )
{
	return currentOffset + currentNumElements == nextOffset;
}

#endif
