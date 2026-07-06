#ifndef RI_OFFSET_ALLOC_H
#define RI_OFFSET_ALLOC_H

// Port of Sebastian Aaltonen's OffsetAllocator (MIT, 2023).
//
// A two-level segregated-fit (TLSF-like) allocator that hands out (offset, size) ranges from a
// fixed-size virtual region. Designed for sub-allocating GPU buffer/heap regions; this manages only
// metadata -- the caller owns the underlying storage. O(1) allocate/free via bin bitmaps.
// Ported from rhi-zig offset_alloc.zig.

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define RI_OFFSET_ALLOC_NO_SPACE 0xffffffffu

struct RIOffsetAllocation_s {
	uint32_t offset;   // RI_OFFSET_ALLOC_NO_SPACE on failure
	uint32_t metadata; // opaque node index, pass back to RIOffsetFree
};

struct RIOffsetAllocReport_s {
	uint32_t totalFreeSpace;
	uint32_t largestFreeRegion;
};

struct RIOffsetAllocNode_s {
	uint32_t dataOffset;
	uint32_t dataSize;
	uint32_t binListPrev;
	uint32_t binListNext;
	uint32_t neighborPrev;
	uint32_t neighborNext;
	bool used;
};

struct RIOffsetAlloc_s {
	uint32_t size;
	uint32_t maxAllocs;
	uint32_t freeStorage;

	uint32_t usedBinsTop;
	uint8_t usedBins[32];        // RI_OFFSET_NUM_TOP_BINS
	uint32_t binIndices[32 * 8]; // RI_OFFSET_NUM_LEAF_BINS

	struct RIOffsetAllocNode_s *nodes;
	uint32_t *freeNodes;
	uint32_t freeOffset;
};

// Initialize an allocator over a [0, size) virtual region, with capacity for maxAllocs live nodes.
void InitRIOffsetAlloc( struct RIOffsetAlloc_s *alloc, uint32_t size, uint32_t maxAllocs );
// Return the allocator to its initial single-free-region state (reallocates the node arrays).
void ResetRIOffsetAlloc( struct RIOffsetAlloc_s *alloc );
void FreeRIOffsetAlloc( struct RIOffsetAlloc_s *alloc );

struct RIOffsetAllocation_s RIOffsetAlloc( struct RIOffsetAlloc_s *alloc, uint32_t size );
void RIOffsetFree( struct RIOffsetAlloc_s *alloc, struct RIOffsetAllocation_s allocation );

uint32_t RIOffsetAllocationSize( const struct RIOffsetAlloc_s *alloc, struct RIOffsetAllocation_s allocation );
struct RIOffsetAllocReport_s RIOffsetStorageReport( const struct RIOffsetAlloc_s *alloc );

#endif
