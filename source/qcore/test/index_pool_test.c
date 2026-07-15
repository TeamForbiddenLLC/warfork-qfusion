#include "../ri_index_pool.h"
#include "utest.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Ported from rhi-zig index_pool.zig tests.

UTEST( index_pool, basic_request_and_exhaust )
{
	struct RIIndexPool_s pool;
	InitRIIndexPool( &pool, 4 );

	EXPECT_EQ( 3u, RIIndexPoolRequest( &pool ) );
	EXPECT_EQ( 2u, RIIndexPoolRequest( &pool ) );
	EXPECT_EQ( 1u, RIIndexPoolRequest( &pool ) );
	EXPECT_EQ( 0u, RIIndexPoolRequest( &pool ) );
	EXPECT_EQ( RI_INDEX_POOL_INVALID, RIIndexPoolRequest( &pool ) );

	FreeRIIndexPool( &pool );
}

UTEST( index_pool, return_coalesces_with_previous )
{
	struct RIIndexPool_s pool;
	InitRIIndexPool( &pool, 8 );

	for( uint32_t i = 0; i < 8; i++ )
		RIIndexPoolRequest( &pool );
	EXPECT_EQ( RI_INDEX_POOL_INVALID, RIIndexPoolRequest( &pool ) );

	RIIndexPoolReturn( &pool, 0 );
	RIIndexPoolReturn( &pool, 1 );
	RIIndexPoolReturn( &pool, 2 );
	EXPECT_EQ( 1u, pool.count );
	EXPECT_EQ( 0u, pool.available[0].start );
	EXPECT_EQ( 2u, pool.available[0].end );

	FreeRIIndexPool( &pool );
}

UTEST( index_pool, return_coalesces_with_next )
{
	struct RIIndexPool_s pool;
	InitRIIndexPool( &pool, 8 );

	for( uint32_t i = 0; i < 8; i++ )
		RIIndexPoolRequest( &pool );

	RIIndexPoolReturn( &pool, 7 );
	RIIndexPoolReturn( &pool, 6 );
	RIIndexPoolReturn( &pool, 5 );
	EXPECT_EQ( 1u, pool.count );
	EXPECT_EQ( 5u, pool.available[0].start );
	EXPECT_EQ( 7u, pool.available[0].end );

	FreeRIIndexPool( &pool );
}

UTEST( index_pool, return_bridges_two_ranges )
{
	struct RIIndexPool_s pool;
	InitRIIndexPool( &pool, 8 );

	for( uint32_t i = 0; i < 8; i++ )
		RIIndexPoolRequest( &pool );

	// Two disjoint ranges: [0,2] and [4,6].
	RIIndexPoolReturn( &pool, 0 );
	RIIndexPoolReturn( &pool, 1 );
	RIIndexPoolReturn( &pool, 2 );
	RIIndexPoolReturn( &pool, 4 );
	RIIndexPoolReturn( &pool, 5 );
	RIIndexPoolReturn( &pool, 6 );
	EXPECT_EQ( 2u, pool.count );

	// Returning 3 bridges the two ranges.
	RIIndexPoolReturn( &pool, 3 );
	EXPECT_EQ( 1u, pool.count );
	EXPECT_EQ( 0u, pool.available[0].start );
	EXPECT_EQ( 6u, pool.available[0].end );

	FreeRIIndexPool( &pool );
}

UTEST( index_pool, isolated_returns_produce_separate_ranges )
{
	struct RIIndexPool_s pool;
	InitRIIndexPool( &pool, 16 );

	for( uint32_t i = 0; i < 16; i++ )
		RIIndexPoolRequest( &pool );

	RIIndexPoolReturn( &pool, 2 );
	RIIndexPoolReturn( &pool, 7 );
	RIIndexPoolReturn( &pool, 12 );

	EXPECT_EQ( 3u, pool.count );
	EXPECT_EQ( 2u, pool.available[0].start );
	EXPECT_EQ( 2u, pool.available[0].end );
	EXPECT_EQ( 7u, pool.available[1].start );
	EXPECT_EQ( 7u, pool.available[1].end );
	EXPECT_EQ( 12u, pool.available[2].start );
	EXPECT_EQ( 12u, pool.available[2].end );

	FreeRIIndexPool( &pool );
}

UTEST( index_pool, reset_to_reserved )
{
	struct RIIndexPool_s pool;
	InitRIIndexPool( &pool, 4 );

	RIIndexPoolRequest( &pool );
	RIIndexPoolRequest( &pool );
	RIIndexPoolResetToReserved( &pool, 8 );

	EXPECT_EQ( 1u, pool.count );
	EXPECT_EQ( 0u, pool.available[0].start );
	EXPECT_EQ( 7u, pool.available[0].end );

	FreeRIIndexPool( &pool );
}

UTEST( index_pool, request_after_return_reuses_id )
{
	struct RIIndexPool_s pool;
	InitRIIndexPool( &pool, 4 );

	const uint32_t a = RIIndexPoolRequest( &pool );
	const uint32_t b = RIIndexPoolRequest( &pool );
	(void)b;
	RIIndexPoolReturn( &pool, a );
	const uint32_t c = RIIndexPoolRequest( &pool );
	// a was the highest; after return it sits at the end of the free list, so it pops first.
	EXPECT_EQ( a, c );

	FreeRIIndexPool( &pool );
}

UTEST( index_pool, stress_round_trip_preserves_uniqueness )
{
	const uint32_t N = 1024;
	struct RIIndexPool_s pool;
	InitRIIndexPool( &pool, N );

	bool *seen = calloc( N, sizeof( bool ) );
	uint32_t *handedOut = malloc( sizeof( uint32_t ) * N );

	for( uint32_t i = 0; i < N; i++ ) {
		const uint32_t id = RIIndexPoolRequest( &pool );
		EXPECT_LT( id, N );
		EXPECT_FALSE( seen[id] );
		seen[id] = true;
		handedOut[i] = id;
	}
	EXPECT_EQ( RI_INDEX_POOL_INVALID, RIIndexPoolRequest( &pool ) );

	// Return everything in a scrambled (deterministic) order via a Fisher-Yates shuffle.
	unsigned int rngState = 0xC0FFEEu;
	for( uint32_t i = N - 1; i > 0; i-- ) {
		rngState = rngState * 1103515245u + 12345u;
		const uint32_t j = ( rngState >> 16 ) % ( i + 1 );
		const uint32_t tmp = handedOut[i];
		handedOut[i] = handedOut[j];
		handedOut[j] = tmp;
	}
	for( uint32_t i = 0; i < N; i++ )
		RIIndexPoolReturn( &pool, handedOut[i] );

	// The free list should have collapsed back to a single full range.
	EXPECT_EQ( 1u, pool.count );
	EXPECT_EQ( 0u, pool.available[0].start );
	EXPECT_EQ( N - 1, pool.available[0].end );

	free( seen );
	free( handedOut );
	FreeRIIndexPool( &pool );
}

UTEST_MAIN();
