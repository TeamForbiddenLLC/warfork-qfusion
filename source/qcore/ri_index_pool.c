#include "ri_index_pool.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

static void __ensureCapacity( struct RIIndexPool_s *pool, uint32_t needed )
{
	if( pool->capacity >= needed )
		return;
	uint32_t newCap = pool->capacity ? pool->capacity * 2 : 4;
	while( newCap < needed )
		newCap *= 2;
	pool->available = realloc( pool->available, sizeof( struct RIIndexRange_s ) * newCap );
	pool->capacity = newCap;
}

static void __insertAt( struct RIIndexPool_s *pool, uint32_t at, struct RIIndexRange_s range )
{
	__ensureCapacity( pool, pool->count + 1 );
	for( uint32_t i = pool->count; i > at; i-- )
		pool->available[i] = pool->available[i - 1];
	pool->available[at] = range;
	pool->count++;
}

static void __removeAt( struct RIIndexPool_s *pool, uint32_t at )
{
	for( uint32_t i = at; i + 1 < pool->count; i++ )
		pool->available[i] = pool->available[i + 1];
	pool->count--;
}

void InitRIIndexPool( struct RIIndexPool_s *pool, uint32_t reserve )
{
	assert( reserve > 0 );
	pool->available = NULL;
	pool->count = 0;
	pool->capacity = 0;
	__ensureCapacity( pool, 1 );
	pool->available[0] = ( struct RIIndexRange_s ){ 0, reserve - 1 };
	pool->count = 1;
}

void FreeRIIndexPool( struct RIIndexPool_s *pool )
{
	free( pool->available );
	pool->available = NULL;
	pool->count = 0;
	pool->capacity = 0;
}

uint32_t RIIndexPoolRequest( struct RIIndexPool_s *pool )
{
	if( pool->count == 0 )
		return RI_INDEX_POOL_INVALID;
	struct RIIndexRange_s *entry = &pool->available[pool->count - 1];
	if( entry->end == entry->start ) {
		const uint32_t res = entry->start;
		pool->count--;
		return res;
	}
	const uint32_t res = entry->end;
	entry->end--;
	return res;
}

void RIIndexPoolReset( struct RIIndexPool_s *pool )
{
	uint32_t max = 0;
	for( uint32_t i = 0; i < pool->count; i++ ) {
		if( pool->available[i].end > max )
			max = pool->available[i].end;
	}
	__ensureCapacity( pool, 1 );
	pool->available[0] = ( struct RIIndexRange_s ){ 0, max };
	pool->count = 1;
}

void RIIndexPoolResetToReserved( struct RIIndexPool_s *pool, uint32_t reserve )
{
	assert( reserve > 0 );
	__ensureCapacity( pool, 1 );
	pool->available[0] = ( struct RIIndexRange_s ){ 0, reserve - 1 };
	pool->count = 1;
}

void RIIndexPoolReturn( struct RIIndexPool_s *pool, uint32_t index )
{
	// Binary search: smallest i such that available[i].start > index, or count if no such i exists.
	uint32_t lo = 0;
	uint32_t hi = pool->count;
	while( lo < hi ) {
		const uint32_t mid = lo + ( hi - lo ) / 2;
		if( pool->available[mid].start > index )
			hi = mid;
		else
			lo = mid + 1;
	}
	const uint32_t insertAt = lo;

	const bool hasPrev = insertAt > 0;
	const bool hasNext = insertAt < pool->count;

	// Duplicate-return detection.
	if( hasPrev )
		assert( pool->available[insertAt - 1].end < index );
	if( hasNext )
		assert( pool->available[insertAt].start > index );

	const bool prevAdj = hasPrev && pool->available[insertAt - 1].end + 1 == index;
	const bool nextAdj = hasNext && pool->available[insertAt].start == index + 1;

	if( prevAdj && nextAdj ) {
		// Merge prev and next into a single range.
		pool->available[insertAt - 1].end = pool->available[insertAt].end;
		__removeAt( pool, insertAt );
	} else if( prevAdj ) {
		pool->available[insertAt - 1].end = index;
	} else if( nextAdj ) {
		pool->available[insertAt].start = index;
	} else {
		__insertAt( pool, insertAt, ( struct RIIndexRange_s ){ index, index } );
	}
}
