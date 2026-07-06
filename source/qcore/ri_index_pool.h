#ifndef RI_INDEX_POOL_H
#define RI_INDEX_POOL_H

// A pool of unique u32 IDs. Hands out IDs from a fixed reserved range and accepts them back. The free
// set is stored as a sorted list of inclusive [start, end] ranges; returns coalesce with adjacent
// ranges so the metadata stays compact even after long allocate/free histories.
//
// Ported from rhi-zig index_pool.zig, itself ported from the C++ hpl::IndexPool (HPL2 / Amnesia).
// The original had an asymmetric coalescing bug and missed some merges on insert; both are fixed here.

#include <stdint.h>

#define RI_INDEX_POOL_INVALID 0xffffffffu

struct RIIndexRange_s {
	uint32_t start; // inclusive
	uint32_t end;   // inclusive
};

struct RIIndexPool_s {
	struct RIIndexRange_s *available; // sorted ascending by start; adjacent ranges coalesce on return
	uint32_t count;
	uint32_t capacity;
};

// Reserve IDs [0, reserve). reserve must be > 0.
void InitRIIndexPool( struct RIIndexPool_s *pool, uint32_t reserve );
void FreeRIIndexPool( struct RIIndexPool_s *pool );

// Pop the highest free ID. Returns RI_INDEX_POOL_INVALID when the pool is empty.
uint32_t RIIndexPoolRequest( struct RIIndexPool_s *pool );
// Return a previously-requested ID. Asserts in debug if the ID is already free.
void RIIndexPoolReturn( struct RIIndexPool_s *pool, uint32_t index );

// Reset to a single range [0, highest endpoint currently in the free list].
void RIIndexPoolReset( struct RIIndexPool_s *pool );
// Reset to a single range [0, reserve). reserve must be > 0.
void RIIndexPoolResetToReserved( struct RIIndexPool_s *pool, uint32_t reserve );

#endif
