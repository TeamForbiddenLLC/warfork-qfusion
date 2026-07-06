#include "ri_offset_alloc.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define RI_OFFSET_NUM_TOP_BINS 32
#define RI_OFFSET_BINS_PER_LEAF 8
#define RI_OFFSET_TOP_BINS_INDEX_SHIFT 3
#define RI_OFFSET_LEAF_BINS_INDEX_MASK 0x7u
#define RI_OFFSET_NUM_LEAF_BINS ( RI_OFFSET_NUM_TOP_BINS * RI_OFFSET_BINS_PER_LEAF )
#define RI_OFFSET_NODE_UNUSED 0xffffffffu

#define RI_OFFSET_MANTISSA_BITS 3
#define RI_OFFSET_MANTISSA_VALUE ( 1u << RI_OFFSET_MANTISSA_BITS )
#define RI_OFFSET_MANTISSA_MASK ( RI_OFFSET_MANTISSA_VALUE - 1 )

// Floating-point-style size->bin mapping. Bins are spaced log-linearly so the worst-case rounding
// overhead is a fixed percentage.
static uint32_t __sf_uintToFloatRoundUp( uint32_t size )
{
	uint32_t exp = 0;
	uint32_t mantissa = 0;
	if( size < RI_OFFSET_MANTISSA_VALUE ) {
		mantissa = size;
	} else {
		const uint32_t highestSetBit = 31u - (uint32_t)__builtin_clz( size );
		const uint32_t mantissaStartBit = highestSetBit - RI_OFFSET_MANTISSA_BITS;
		exp = mantissaStartBit + 1;
		mantissa = ( size >> mantissaStartBit ) & RI_OFFSET_MANTISSA_MASK;

		const uint32_t lowBitsMask = ( 1u << mantissaStartBit ) - 1;
		if( ( size & lowBitsMask ) != 0 )
			mantissa++;
	}
	// '+' (not '|') so a mantissa overflow carries into the exponent.
	return ( exp << RI_OFFSET_MANTISSA_BITS ) + mantissa;
}

static uint32_t __sf_uintToFloatRoundDown( uint32_t size )
{
	uint32_t exp = 0;
	uint32_t mantissa = 0;
	if( size < RI_OFFSET_MANTISSA_VALUE ) {
		mantissa = size;
	} else {
		const uint32_t highestSetBit = 31u - (uint32_t)__builtin_clz( size );
		const uint32_t mantissaStartBit = highestSetBit - RI_OFFSET_MANTISSA_BITS;
		exp = mantissaStartBit + 1;
		mantissa = ( size >> mantissaStartBit ) & RI_OFFSET_MANTISSA_MASK;
	}
	return ( exp << RI_OFFSET_MANTISSA_BITS ) | mantissa;
}

static uint32_t __sf_floatToUint( uint32_t floatValue )
{
	const uint32_t exponent = floatValue >> RI_OFFSET_MANTISSA_BITS;
	const uint32_t mantissa = floatValue & RI_OFFSET_MANTISSA_MASK;
	if( exponent == 0 )
		return mantissa;
	return ( mantissa | RI_OFFSET_MANTISSA_VALUE ) << ( exponent - 1 );
}

static uint32_t __findLowestSetBitAfter( uint32_t bitMask, uint32_t startBitIndex )
{
	// The C++ original relies on '1 << 32' UB wrapping to 1 on x86; guard explicitly.
	if( startBitIndex >= 32 )
		return RI_OFFSET_ALLOC_NO_SPACE;
	const uint32_t maskBefore = ( 1u << startBitIndex ) - 1;
	const uint32_t maskAfter = ~maskBefore;
	const uint32_t bitsAfter = bitMask & maskAfter;
	if( bitsAfter == 0 )
		return RI_OFFSET_ALLOC_NO_SPACE;
	return (uint32_t)__builtin_ctz( bitsAfter );
}

static uint32_t __insertNodeIntoBin( struct RIOffsetAlloc_s *a, uint32_t sz, uint32_t dataOffset )
{
	// Round down: pick the largest bin the size is still guaranteed to satisfy (so any allocation
	// rounded up into this bin will fit).
	const uint32_t binIndex = __sf_uintToFloatRoundDown( sz );
	const uint32_t topBinIndex = binIndex >> RI_OFFSET_TOP_BINS_INDEX_SHIFT;
	const uint32_t leafBinIndex = binIndex & RI_OFFSET_LEAF_BINS_INDEX_MASK;

	if( a->binIndices[binIndex] == RI_OFFSET_NODE_UNUSED ) {
		a->usedBins[topBinIndex] |= (uint8_t)( 1u << leafBinIndex );
		a->usedBinsTop |= ( 1u << topBinIndex );
	}

	const uint32_t topNodeIndex = a->binIndices[binIndex];
	const uint32_t nodeIndex = a->freeNodes[a->freeOffset];
	// Unsigned wrap matches the C++ original; the freeOffset==0 guard in RIOffsetAlloc keeps this from
	// being read back as an out-of-bounds index.
	a->freeOffset--;

	struct RIOffsetAllocNode_s *n = &a->nodes[nodeIndex];
	n->dataOffset = dataOffset;
	n->dataSize = sz;
	n->binListPrev = RI_OFFSET_NODE_UNUSED;
	n->binListNext = topNodeIndex;
	n->neighborPrev = RI_OFFSET_NODE_UNUSED;
	n->neighborNext = RI_OFFSET_NODE_UNUSED;
	n->used = false;

	if( topNodeIndex != RI_OFFSET_NODE_UNUSED )
		a->nodes[topNodeIndex].binListPrev = nodeIndex;
	a->binIndices[binIndex] = nodeIndex;

	a->freeStorage += sz;
	return nodeIndex;
}

static void __removeNodeFromBin( struct RIOffsetAlloc_s *a, uint32_t nodeIndex )
{
	const uint32_t nodeDataSize = a->nodes[nodeIndex].dataSize;
	const uint32_t nodeBinPrev = a->nodes[nodeIndex].binListPrev;
	const uint32_t nodeBinNext = a->nodes[nodeIndex].binListNext;

	if( nodeBinPrev != RI_OFFSET_NODE_UNUSED ) {
		// Middle of a bin's linked list -- just unlink.
		a->nodes[nodeBinPrev].binListNext = nodeBinNext;
		if( nodeBinNext != RI_OFFSET_NODE_UNUSED )
			a->nodes[nodeBinNext].binListPrev = nodeBinPrev;
	} else {
		// Head of the list -- also update the bin-occupancy bitmasks.
		const uint32_t binIndex = __sf_uintToFloatRoundDown( nodeDataSize );
		const uint32_t topBinIndex = binIndex >> RI_OFFSET_TOP_BINS_INDEX_SHIFT;
		const uint32_t leafBinIndex = binIndex & RI_OFFSET_LEAF_BINS_INDEX_MASK;

		a->binIndices[binIndex] = nodeBinNext;
		if( nodeBinNext != RI_OFFSET_NODE_UNUSED )
			a->nodes[nodeBinNext].binListPrev = RI_OFFSET_NODE_UNUSED;

		if( a->binIndices[binIndex] == RI_OFFSET_NODE_UNUSED ) {
			a->usedBins[topBinIndex] &= (uint8_t)~( 1u << leafBinIndex );
			if( a->usedBins[topBinIndex] == 0 )
				a->usedBinsTop &= ~( 1u << topBinIndex );
		}
	}

	a->freeOffset++;
	a->freeNodes[a->freeOffset] = nodeIndex;
	a->freeStorage -= nodeDataSize;
}

void ResetRIOffsetAlloc( struct RIOffsetAlloc_s *alloc )
{
	alloc->freeStorage = 0;
	alloc->usedBinsTop = 0;
	alloc->freeOffset = alloc->maxAllocs - 1;

	memset( alloc->usedBins, 0, sizeof( alloc->usedBins ) );
	for( uint32_t i = 0; i < RI_OFFSET_NUM_LEAF_BINS; i++ )
		alloc->binIndices[i] = RI_OFFSET_NODE_UNUSED;

	free( alloc->nodes );
	free( alloc->freeNodes );
	alloc->nodes = malloc( sizeof( struct RIOffsetAllocNode_s ) * alloc->maxAllocs );
	alloc->freeNodes = malloc( sizeof( uint32_t ) * alloc->maxAllocs );

	// Stack of free indices -- pop from the top so node 0 comes out first.
	for( uint32_t i = 0; i < alloc->maxAllocs; i++ )
		alloc->freeNodes[i] = alloc->maxAllocs - i - 1;

	// The whole region begins as a single free node spanning [0, size).
	__insertNodeIntoBin( alloc, alloc->size, 0 );
}

void InitRIOffsetAlloc( struct RIOffsetAlloc_s *alloc, uint32_t size, uint32_t maxAllocs )
{
	memset( alloc, 0, sizeof( struct RIOffsetAlloc_s ) );
	alloc->size = size;
	alloc->maxAllocs = maxAllocs;
	alloc->nodes = NULL;
	alloc->freeNodes = NULL;
	ResetRIOffsetAlloc( alloc );
}

void FreeRIOffsetAlloc( struct RIOffsetAlloc_s *alloc )
{
	free( alloc->nodes );
	free( alloc->freeNodes );
	alloc->nodes = NULL;
	alloc->freeNodes = NULL;
}

struct RIOffsetAllocation_s RIOffsetAlloc( struct RIOffsetAlloc_s *alloc, uint32_t size )
{
	struct RIOffsetAllocation_s result = { RI_OFFSET_ALLOC_NO_SPACE, RI_OFFSET_ALLOC_NO_SPACE };
	if( alloc->freeOffset == 0 )
		return result;

	// Round up: smallest bin guaranteed to fit `size`.
	const uint32_t minBinIndex = __sf_uintToFloatRoundUp( size );
	const uint32_t minTopBinIndex = minBinIndex >> RI_OFFSET_TOP_BINS_INDEX_SHIFT;
	const uint32_t minLeafBinIndex = minBinIndex & RI_OFFSET_LEAF_BINS_INDEX_MASK;

	uint32_t topBinIndex = minTopBinIndex;
	uint32_t leafBinIndex = RI_OFFSET_ALLOC_NO_SPACE;

	// Probe the rounded-up bin first (the rounded-up leaf within this top bin can be empty even when
	// higher leaves are not).
	if( ( alloc->usedBinsTop & ( 1u << topBinIndex ) ) != 0 )
		leafBinIndex = __findLowestSetBitAfter( (uint32_t)alloc->usedBins[topBinIndex], minLeafBinIndex );

	// Fall back to the next non-empty top bin (any leaf there fits).
	if( leafBinIndex == RI_OFFSET_ALLOC_NO_SPACE ) {
		topBinIndex = __findLowestSetBitAfter( alloc->usedBinsTop, minTopBinIndex + 1 );
		if( topBinIndex == RI_OFFSET_ALLOC_NO_SPACE )
			return result;
		leafBinIndex = (uint32_t)__builtin_ctz( alloc->usedBins[topBinIndex] );
	}

	const uint32_t binIndex = ( topBinIndex << RI_OFFSET_TOP_BINS_INDEX_SHIFT ) | leafBinIndex;

	// Pop the head of the bin's linked list.
	const uint32_t nodeIndex = alloc->binIndices[binIndex];
	struct RIOffsetAllocNode_s *node = &alloc->nodes[nodeIndex];
	const uint32_t nodeTotalSize = node->dataSize;
	const uint32_t nodeDataOffset = node->dataOffset;
	const uint32_t nodeNeighborNext = node->neighborNext;

	node->dataSize = size;
	node->used = true;
	alloc->binIndices[binIndex] = node->binListNext;
	if( node->binListNext != RI_OFFSET_NODE_UNUSED )
		alloc->nodes[node->binListNext].binListPrev = RI_OFFSET_NODE_UNUSED;
	alloc->freeStorage -= nodeTotalSize;

	// Update bin-occupancy bitmasks if the bin is now empty.
	if( alloc->binIndices[binIndex] == RI_OFFSET_NODE_UNUSED ) {
		alloc->usedBins[topBinIndex] &= (uint8_t)~( 1u << leafBinIndex );
		if( alloc->usedBins[topBinIndex] == 0 )
			alloc->usedBinsTop &= ~( 1u << topBinIndex );
	}

	// Push the unused tail back into the freelist as a smaller free node.
	const uint32_t reminderSize = nodeTotalSize - size;
	if( reminderSize > 0 ) {
		const uint32_t newNodeIndex = __insertNodeIntoBin( alloc, reminderSize, nodeDataOffset + size );

		// Splice the new node between `node` and its old next neighbor.
		if( nodeNeighborNext != RI_OFFSET_NODE_UNUSED )
			alloc->nodes[nodeNeighborNext].neighborPrev = newNodeIndex;
		alloc->nodes[newNodeIndex].neighborPrev = nodeIndex;
		alloc->nodes[newNodeIndex].neighborNext = nodeNeighborNext;
		alloc->nodes[nodeIndex].neighborNext = newNodeIndex;
	}

	result.offset = nodeDataOffset;
	result.metadata = nodeIndex;
	return result;
}

void RIOffsetFree( struct RIOffsetAlloc_s *alloc, struct RIOffsetAllocation_s allocation )
{
	assert( allocation.metadata != RI_OFFSET_ALLOC_NO_SPACE );
	if( alloc->nodes == NULL )
		return;

	const uint32_t nodeIndex = allocation.metadata;
	assert( alloc->nodes[nodeIndex].used );

	uint32_t offset = alloc->nodes[nodeIndex].dataOffset;
	uint32_t sz = alloc->nodes[nodeIndex].dataSize;

	// Coalesce with previous neighbor if it's free.
	const uint32_t prevNeighbor = alloc->nodes[nodeIndex].neighborPrev;
	if( prevNeighbor != RI_OFFSET_NODE_UNUSED && !alloc->nodes[prevNeighbor].used ) {
		offset = alloc->nodes[prevNeighbor].dataOffset;
		sz += alloc->nodes[prevNeighbor].dataSize;
		__removeNodeFromBin( alloc, prevNeighbor );
		assert( alloc->nodes[prevNeighbor].neighborNext == nodeIndex );
		alloc->nodes[nodeIndex].neighborPrev = alloc->nodes[prevNeighbor].neighborPrev;
	}

	// Coalesce with next neighbor if it's free.
	const uint32_t nextNeighbor = alloc->nodes[nodeIndex].neighborNext;
	if( nextNeighbor != RI_OFFSET_NODE_UNUSED && !alloc->nodes[nextNeighbor].used ) {
		sz += alloc->nodes[nextNeighbor].dataSize;
		__removeNodeFromBin( alloc, nextNeighbor );
		assert( alloc->nodes[nextNeighbor].neighborPrev == nodeIndex );
		alloc->nodes[nodeIndex].neighborNext = alloc->nodes[nextNeighbor].neighborNext;
	}

	const uint32_t finalNeighborNext = alloc->nodes[nodeIndex].neighborNext;
	const uint32_t finalNeighborPrev = alloc->nodes[nodeIndex].neighborPrev;

	// Return the freed node to the freelist.
	alloc->freeOffset++;
	alloc->freeNodes[alloc->freeOffset] = nodeIndex;

	// Re-insert the (possibly coalesced) free range as a new bin node.
	const uint32_t combinedNodeIndex = __insertNodeIntoBin( alloc, sz, offset );

	if( finalNeighborNext != RI_OFFSET_NODE_UNUSED ) {
		alloc->nodes[combinedNodeIndex].neighborNext = finalNeighborNext;
		alloc->nodes[finalNeighborNext].neighborPrev = combinedNodeIndex;
	}
	if( finalNeighborPrev != RI_OFFSET_NODE_UNUSED ) {
		alloc->nodes[combinedNodeIndex].neighborPrev = finalNeighborPrev;
		alloc->nodes[finalNeighborPrev].neighborNext = combinedNodeIndex;
	}
}

uint32_t RIOffsetAllocationSize( const struct RIOffsetAlloc_s *alloc, struct RIOffsetAllocation_s allocation )
{
	if( allocation.metadata == RI_OFFSET_ALLOC_NO_SPACE )
		return 0;
	if( alloc->nodes == NULL )
		return 0;
	return alloc->nodes[allocation.metadata].dataSize;
}

struct RIOffsetAllocReport_s RIOffsetStorageReport( const struct RIOffsetAlloc_s *alloc )
{
	struct RIOffsetAllocReport_s report = { 0, 0 };
	if( alloc->freeOffset > 0 ) {
		report.totalFreeSpace = alloc->freeStorage;
		if( alloc->usedBinsTop != 0 ) {
			const uint32_t topBinIndex = 31u - (uint32_t)__builtin_clz( alloc->usedBinsTop );
			const uint32_t leafBinIndex = 31u - (uint32_t)__builtin_clz( (uint32_t)alloc->usedBins[topBinIndex] );
			report.largestFreeRegion = __sf_floatToUint( ( topBinIndex << RI_OFFSET_TOP_BINS_INDEX_SHIFT ) | leafBinIndex );
			assert( report.totalFreeSpace >= report.largestFreeRegion );
		}
	}
	return report;
}
