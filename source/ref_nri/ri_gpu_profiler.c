#include "ri_gpu_profiler.h"
#include "stb_ds.h"

void InitRIGpuProfiler( struct RIDevice_s *dev, uint32_t numSlots, struct RIGpuProfiler_s *profiler )
{
	memset( profiler, 0, sizeof( struct RIGpuProfiler_s ) );
	assert( numSlots > 0 && numSlots <= RI_GPU_PROFILER_MAX_SLOTS );
	profiler->numSlots = numSlots;
	profiler->activeSlot = 0;
	profiler->enabled = false;
	profiler->ticksToMs = 0;
	profiler->validBitsMask = ~(uint64_t)0;
	profiler->totalMs = 0;
	for( uint32_t i = 0; i < numSlots; i++ ) {
		profiler->slots[i].timelineValue = 0;
		profiler->slots[i].resolved = true;
		profiler->slots[i].queryCount = 0;
		profiler->slots[i].scopes = NULL;
	}
#if ( DEVICE_IMPL_VULKAN )
	{
		uint32_t familyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( dev->physicalAdapter.vk.physicalDevice, &familyCount, NULL );
		VkQueueFamilyProperties *familyProps = malloc( sizeof( VkQueueFamilyProperties ) * familyCount );
		vkGetPhysicalDeviceQueueFamilyProperties( dev->physicalAdapter.vk.physicalDevice, &familyCount, familyProps );

		const uint32_t gfxFamily = dev->queues[RI_QUEUE_GRAPHICS].vk.queueFamilyIdx;
		const uint32_t validBits = ( gfxFamily < familyCount ) ? familyProps[gfxFamily].timestampValidBits : 0;
		free( familyProps );

		profiler->validBitsMask = ( validBits >= 64 ) ? ~(uint64_t)0 : ( ( (uint64_t)1 << validBits ) - 1 );

		// A timestampValidBits of 0 means the graphics queue does not support timestamp queries.
		if( validBits == 0 || dev->physicalAdapter.timestampFrequencyHz == 0 ) {
			profiler->enabled = false;
			return;
		}
		profiler->ticksToMs = 1000.0 / (double)dev->physicalAdapter.timestampFrequencyHz;

		VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
		createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		createInfo.queryCount = RI_GPU_PROFILER_MAX_QUERIES;
		for( uint32_t i = 0; i < numSlots; i++ ) {
			if( !VK_WrapResult( vkCreateQueryPool( dev->vk.device, &createInfo, NULL, &profiler->slots[i].vk.pool ) ) )
				return;
		}
		profiler->enabled = true;
	}
#endif
}

void FreeRIGpuProfiler( struct RIDevice_s *dev, struct RIGpuProfiler_s *profiler )
{
	for( uint32_t i = 0; i < profiler->numSlots; i++ ) {
#if ( DEVICE_IMPL_VULKAN )
		if( profiler->slots[i].vk.pool )
			vkDestroyQueryPool( dev->vk.device, profiler->slots[i].vk.pool, NULL );
#endif
		arrfree( profiler->slots[i].scopes );
	}
	arrfree( profiler->openStack );
	arrfree( profiler->results );
	memset( profiler, 0, sizeof( struct RIGpuProfiler_s ) );
}

void RIGpuProfilerBeginFrame( struct RIDevice_s *dev, struct RIGpuProfiler_s *p, struct RICmd_s *cmd, uint32_t slot, uint64_t timelineValue )
{
	if( !p->enabled )
		return;
	assert( slot < p->numSlots );

	p->activeSlot = slot;
	struct RIGpuProfilerSlot_s *s = &p->slots[slot];
	s->timelineValue = timelineValue;
	s->resolved = false;
	s->queryCount = 0;
	arrsetlen( s->scopes, 0 );
	arrsetlen( p->openStack, 0 );

#if ( DEVICE_IMPL_VULKAN )
	vkCmdResetQueryPool( cmd->vk.cmd, s->vk.pool, 0, RI_GPU_PROFILER_MAX_QUERIES );
#endif
}

void RIGpuProfilerBeginScope( struct RIDevice_s *dev, struct RIGpuProfiler_s *p, struct RICmd_s *cmd, const char *name )
{
	if( !p->enabled )
		return;

	struct RIGpuProfilerSlot_s *s = &p->slots[p->activeSlot];
	const uint32_t depth = (uint32_t)arrlen( p->openStack );
	const uint32_t scopeIdx = (uint32_t)arrlen( s->scopes );

	uint32_t beginIdx = RI_GPU_PROFILER_INVALID_QUERY;
	uint32_t endIdx = RI_GPU_PROFILER_INVALID_QUERY;

	// Gracefully degrade once the per-frame query budget is exhausted: the scope is still recorded
	// (so nesting stays consistent) but resolves to 0 ms.
	if( s->queryCount + 2 <= RI_GPU_PROFILER_MAX_QUERIES ) {
		beginIdx = s->queryCount;
		endIdx = s->queryCount + 1;
		s->queryCount += 2;
#if ( DEVICE_IMPL_VULKAN )
		vkCmdWriteTimestamp2( cmd->vk.cmd, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, s->vk.pool, beginIdx );
#endif
	}

	struct RIGpuProfilerScope_s scope = { .name = name, .depth = depth, .beginIdx = beginIdx, .endIdx = endIdx };
	arrpush( s->scopes, scope );
	arrpush( p->openStack, scopeIdx );
}

void RIGpuProfilerEndScope( struct RIDevice_s *dev, struct RIGpuProfiler_s *p, struct RICmd_s *cmd )
{
	if( !p->enabled )
		return;
	if( arrlen( p->openStack ) == 0 )
		return;

	const uint32_t scopeIdx = arrpop( p->openStack );
	struct RIGpuProfilerSlot_s *s = &p->slots[p->activeSlot];
	struct RIGpuProfilerScope_s *scope = &s->scopes[scopeIdx];

	if( scope->endIdx != RI_GPU_PROFILER_INVALID_QUERY ) {
#if ( DEVICE_IMPL_VULKAN )
		vkCmdWriteTimestamp2( cmd->vk.cmd, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, s->vk.pool, scope->endIdx );
#endif
	}
}

void RIGpuProfilerResolve( struct RIDevice_s *dev, struct RIGpuProfiler_s *p, uint64_t completedTimeline )
{
	if( !p->enabled )
		return;

	arrsetlen( p->results, 0 );
	p->totalMs = 0;

#if ( DEVICE_IMPL_VULKAN )
	for( uint32_t i = 0; i < p->numSlots; i++ ) {
		struct RIGpuProfilerSlot_s *s = &p->slots[i];
		if( s->resolved || s->timelineValue > completedTimeline || s->queryCount == 0 )
			continue;

		uint64_t timestamps[RI_GPU_PROFILER_MAX_QUERIES];
		VkResult vkResult = vkGetQueryPoolResults( dev->vk.device, s->vk.pool, 0, s->queryCount, s->queryCount * sizeof( uint64_t ), timestamps, sizeof( uint64_t ),
												   VK_QUERY_RESULT_64_BIT );
		if( vkResult == VK_NOT_READY )
			continue;
		if( vkResult != VK_SUCCESS ) {
			VK_WrapResult( vkResult );
			continue;
		}

		double depth0Total = 0;
		for( size_t j = 0; j < arrlen( s->scopes ); j++ ) {
			const struct RIGpuProfilerScope_s *scope = &s->scopes[j];
			if( scope->beginIdx == RI_GPU_PROFILER_INVALID_QUERY ) {
				struct RIGpuPassTiming_s timing = { .name = scope->name, .ms = 0, .depth = scope->depth };
				arrpush( p->results, timing );
				continue;
			}
			const uint64_t ticks = ( timestamps[scope->endIdx] - timestamps[scope->beginIdx] ) & p->validBitsMask;
			const double ms = (double)ticks * p->ticksToMs;
			struct RIGpuPassTiming_s timing = { .name = scope->name, .ms = (float)ms, .depth = scope->depth };
			arrpush( p->results, timing );
			if( scope->depth == 0 )
				depth0Total += ms;
		}
		p->totalMs = (float)depth0Total;
		s->resolved = true;
		break; // resolve at most one slot per call
	}
#endif
}
