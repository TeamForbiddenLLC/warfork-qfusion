#include "../ri_offset_alloc.h"
#include "utest.h"

// Ported from rhi-zig offset_alloc.zig tests (originally offsetAllocatorTests.cpp). The internal
// SmallFloat bin math is validated indirectly: the exact offsets asserted below only hold if the
// round-up/round-down bin mapping is correct.

#define OA_SIZE ( 1024u * 1024u * 256u )
#define OA_MAX_ALLOCS ( 128u * 1024u )

UTEST( offset_alloc, basic )
{
	struct RIOffsetAlloc_s a;
	InitRIOffsetAlloc( &a, OA_SIZE, OA_MAX_ALLOCS );

	struct RIOffsetAllocation_s x = RIOffsetAlloc( &a, 1337 );
	EXPECT_EQ( 0u, x.offset );
	RIOffsetFree( &a, x );

	FreeRIOffsetAlloc( &a );
}

UTEST( offset_alloc, allocate_simple )
{
	struct RIOffsetAlloc_s a;
	InitRIOffsetAlloc( &a, OA_SIZE, OA_MAX_ALLOCS );

	struct RIOffsetAllocation_s x0 = RIOffsetAlloc( &a, 0 );
	EXPECT_EQ( 0u, x0.offset );
	struct RIOffsetAllocation_s x1 = RIOffsetAlloc( &a, 1 );
	EXPECT_EQ( 0u, x1.offset );
	struct RIOffsetAllocation_s x2 = RIOffsetAlloc( &a, 123 );
	EXPECT_EQ( 1u, x2.offset );
	struct RIOffsetAllocation_s x3 = RIOffsetAlloc( &a, 1234 );
	EXPECT_EQ( 124u, x3.offset );

	RIOffsetFree( &a, x0 );
	RIOffsetFree( &a, x1 );
	RIOffsetFree( &a, x2 );
	RIOffsetFree( &a, x3 );

	struct RIOffsetAllocation_s all = RIOffsetAlloc( &a, OA_SIZE );
	EXPECT_EQ( 0u, all.offset );
	RIOffsetFree( &a, all );

	FreeRIOffsetAlloc( &a );
}

UTEST( offset_alloc, merge_trivial )
{
	struct RIOffsetAlloc_s a;
	InitRIOffsetAlloc( &a, OA_SIZE, OA_MAX_ALLOCS );

	struct RIOffsetAllocation_s x = RIOffsetAlloc( &a, 1337 );
	EXPECT_EQ( 0u, x.offset );
	RIOffsetFree( &a, x );

	struct RIOffsetAllocation_s y = RIOffsetAlloc( &a, 1337 );
	EXPECT_EQ( 0u, y.offset );
	RIOffsetFree( &a, y );

	struct RIOffsetAllocation_s all = RIOffsetAlloc( &a, OA_SIZE );
	EXPECT_EQ( 0u, all.offset );
	RIOffsetFree( &a, all );

	FreeRIOffsetAlloc( &a );
}

UTEST( offset_alloc, reuse_trivial )
{
	struct RIOffsetAlloc_s a;
	InitRIOffsetAlloc( &a, OA_SIZE, OA_MAX_ALLOCS );

	struct RIOffsetAllocation_s x = RIOffsetAlloc( &a, 1024 );
	EXPECT_EQ( 0u, x.offset );
	struct RIOffsetAllocation_s y = RIOffsetAlloc( &a, 3456 );
	EXPECT_EQ( 1024u, y.offset );

	RIOffsetFree( &a, x );

	struct RIOffsetAllocation_s z = RIOffsetAlloc( &a, 1024 );
	EXPECT_EQ( 0u, z.offset );

	RIOffsetFree( &a, z );
	RIOffsetFree( &a, y );

	struct RIOffsetAllocation_s all = RIOffsetAlloc( &a, OA_SIZE );
	EXPECT_EQ( 0u, all.offset );
	RIOffsetFree( &a, all );

	FreeRIOffsetAlloc( &a );
}

UTEST( offset_alloc, reuse_complex )
{
	struct RIOffsetAlloc_s a;
	InitRIOffsetAlloc( &a, OA_SIZE, OA_MAX_ALLOCS );

	struct RIOffsetAllocation_s x = RIOffsetAlloc( &a, 1024 );
	EXPECT_EQ( 0u, x.offset );
	struct RIOffsetAllocation_s y = RIOffsetAlloc( &a, 3456 );
	EXPECT_EQ( 1024u, y.offset );

	RIOffsetFree( &a, x );

	struct RIOffsetAllocation_s c = RIOffsetAlloc( &a, 2345 );
	EXPECT_EQ( 1024u + 3456u, c.offset );
	struct RIOffsetAllocation_s d = RIOffsetAlloc( &a, 456 );
	EXPECT_EQ( 0u, d.offset );
	struct RIOffsetAllocation_s e = RIOffsetAlloc( &a, 512 );
	EXPECT_EQ( 456u, e.offset );

	struct RIOffsetAllocReport_s report = RIOffsetStorageReport( &a );
	EXPECT_EQ( OA_SIZE - 3456u - 2345u - 456u - 512u, report.totalFreeSpace );
	EXPECT_NE( report.largestFreeRegion, report.totalFreeSpace );

	RIOffsetFree( &a, c );
	RIOffsetFree( &a, d );
	RIOffsetFree( &a, y );
	RIOffsetFree( &a, e );

	struct RIOffsetAllocation_s all = RIOffsetAlloc( &a, OA_SIZE );
	EXPECT_EQ( 0u, all.offset );
	RIOffsetFree( &a, all );

	FreeRIOffsetAlloc( &a );
}

UTEST( offset_alloc, zero_fragmentation )
{
	struct RIOffsetAlloc_s a;
	InitRIOffsetAlloc( &a, OA_SIZE, OA_MAX_ALLOCS );

	struct RIOffsetAllocation_s allocations[256];
	for( uint32_t i = 0; i < 256; i++ ) {
		allocations[i] = RIOffsetAlloc( &a, 1024 * 1024 );
		EXPECT_EQ( i * 1024u * 1024u, allocations[i].offset );
	}

	{
		struct RIOffsetAllocReport_s report = RIOffsetStorageReport( &a );
		EXPECT_EQ( 0u, report.totalFreeSpace );
		EXPECT_EQ( 0u, report.largestFreeRegion );
	}

	// Free four random slots.
	RIOffsetFree( &a, allocations[243] );
	RIOffsetFree( &a, allocations[5] );
	RIOffsetFree( &a, allocations[123] );
	RIOffsetFree( &a, allocations[95] );

	// Free four contiguous slots -- must coalesce.
	RIOffsetFree( &a, allocations[151] );
	RIOffsetFree( &a, allocations[152] );
	RIOffsetFree( &a, allocations[153] );
	RIOffsetFree( &a, allocations[154] );

	allocations[243] = RIOffsetAlloc( &a, 1024 * 1024 );
	allocations[5] = RIOffsetAlloc( &a, 1024 * 1024 );
	allocations[123] = RIOffsetAlloc( &a, 1024 * 1024 );
	allocations[95] = RIOffsetAlloc( &a, 1024 * 1024 );
	allocations[151] = RIOffsetAlloc( &a, 1024 * 1024 * 4 ); // 4x larger
	EXPECT_NE( RI_OFFSET_ALLOC_NO_SPACE, allocations[243].offset );
	EXPECT_NE( RI_OFFSET_ALLOC_NO_SPACE, allocations[5].offset );
	EXPECT_NE( RI_OFFSET_ALLOC_NO_SPACE, allocations[123].offset );
	EXPECT_NE( RI_OFFSET_ALLOC_NO_SPACE, allocations[95].offset );
	EXPECT_NE( RI_OFFSET_ALLOC_NO_SPACE, allocations[151].offset );

	for( uint32_t i = 0; i < 256; i++ ) {
		if( i < 152 || i > 154 )
			RIOffsetFree( &a, allocations[i] );
	}

	{
		struct RIOffsetAllocReport_s report = RIOffsetStorageReport( &a );
		EXPECT_EQ( OA_SIZE, report.totalFreeSpace );
		EXPECT_EQ( OA_SIZE, report.largestFreeRegion );
	}

	struct RIOffsetAllocation_s all = RIOffsetAlloc( &a, OA_SIZE );
	EXPECT_EQ( 0u, all.offset );
	RIOffsetFree( &a, all );

	FreeRIOffsetAlloc( &a );
}

UTEST_MAIN();
