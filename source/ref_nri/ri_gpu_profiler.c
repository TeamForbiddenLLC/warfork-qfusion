#include "../qcommon/qcommon.h"
#include "ri_gpu_profiler.h"
#include "stb_ds.h"

// Timestamps are taken at ALL_COMMANDS: TOP_OF_PIPE for the begin isn't ordered after earlier work, so a
// scope would absorb the tail of whatever was still in flight before it.
#define RI_GPU_PROFILER_TIMESTAMP_STAGE VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT

void InitRIGpuProfiler( struct RIDevice_s *dev, uint32_t numSlots, struct RIGpuProfiler_s *profiler )
{
	if( profiler->numSlots > 0 )
		FreeRIGpuProfiler( dev, profiler );
	memset( profiler, 0, sizeof( struct RIGpuProfiler_s ) );
	assert( numSlots > 0 && numSlots <= RI_GPU_PROFILER_MAX_SLOTS );
	// clamp in release too: slots[] is fixed size
	numSlots = bound( 1, numSlots, RI_GPU_PROFILER_MAX_SLOTS );
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
	if( RIIsTargetSelected( RI_DEVICE_API_VK ) ) {
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
	}
#endif
}

void FreeRIGpuProfiler( struct RIDevice_s *dev, struct RIGpuProfiler_s *profiler )
{
	for( uint32_t i = 0; i < profiler->numSlots; i++ ) {
#if ( DEVICE_IMPL_VULKAN )
		if( RIIsTargetSelected( RI_DEVICE_API_VK ) ) {
			if( profiler->slots[i].vk.pool )
				vkDestroyQueryPool( dev->vk.device, profiler->slots[i].vk.pool, NULL );
		}
#endif
		arrfree( profiler->slots[i].scopes );
	}
	arrfree( profiler->openStack );
	arrfree( profiler->results );
	memset( profiler, 0, sizeof( struct RIGpuProfiler_s ) );
}

void RIGpuProfilerBeginFrame( struct RIDevice_s *dev, struct RIGpuProfiler_s *p, struct RICmd_s *cmd, uint32_t slot, uint64_t timelineValue )
{
	if( !p->enabled || !cmd )
		return;
	assert( slot < p->numSlots );
	if( slot >= p->numSlots )
		return;

	p->activeSlot = slot;
	struct RIGpuProfilerSlot_s *s = &p->slots[slot];
	s->timelineValue = timelineValue;
	s->resolved = false;
	s->queryCount = 0;
	arrsetlen( s->scopes, 0 );
	arrsetlen( p->openStack, 0 );
	p->openLabels = 0;

#if ( DEVICE_IMPL_VULKAN )
	if( RIIsTargetSelected( RI_DEVICE_API_VK ) ) {
		vkCmdResetQueryPool( cmd->vk.cmd, s->vk.pool, 0, RI_GPU_PROFILER_MAX_QUERIES );
	}
#endif
}

void RIGpuProfilerAbandonFrame( struct RIGpuProfiler_s *p )
{
	if( !p->enabled )
		return;
	// Its timeline value will never be signalled, so resolving it would wait forever / read unwritten queries.
	p->slots[p->activeSlot].resolved = true;
	arrsetlen( p->openStack, 0 );
	p->openLabels = 0;
}

void RIGpuProfilerBeginScope( struct RIDevice_s *dev, struct RIGpuProfiler_s *p, struct RICmd_s *cmd, const char *name )
{
	if( !p->enabled || !cmd )
		return;

	struct RIGpuProfilerSlot_s *s = &p->slots[p->activeSlot];
	const uint32_t depth = (uint32_t)arrlen( p->openStack );
	const uint32_t scopeIdx = (uint32_t)arrlen( s->scopes );

	struct RIGpuProfilerScope_s scope = { .depth = depth, .beginIdx = RI_GPU_PROFILER_INVALID_QUERY, .endIdx = RI_GPU_PROFILER_INVALID_QUERY };
	Q_strncpyz( scope.name, name ? name : "", sizeof( scope.name ) );

	// Only the begin index is taken now; the end index is assigned when the scope closes, so every index
	// below queryCount is actually written and an unclosed scope can't leave a hole that makes the whole
	// range report VK_NOT_READY forever. Once the budget runs out the scope is still recorded (so nesting
	// stays consistent) but resolves to 0 ms.
	if( s->queryCount < RI_GPU_PROFILER_MAX_QUERIES ) {
		scope.beginIdx = s->queryCount++;
#if ( DEVICE_IMPL_VULKAN )
		if( RIIsTargetSelected( RI_DEVICE_API_VK ) ) {
			vkCmdWriteTimestamp2( cmd->vk.cmd, RI_GPU_PROFILER_TIMESTAMP_STAGE, s->vk.pool, scope.beginIdx );
		}
#endif
	}

#if ( DEVICE_IMPL_VULKAN )
	if( RIIsTargetSelected( RI_DEVICE_API_VK ) && vkCmdBeginDebugUtilsLabelEXT ) {
		VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
		label.pLabelName = scope.name;
		vkCmdBeginDebugUtilsLabelEXT( cmd->vk.cmd, &label );
		p->openLabels++;
	}
#endif

	arrpush( s->scopes, scope );
	arrpush( p->openStack, scopeIdx );
}

void RIGpuProfilerEndScope( struct RIDevice_s *dev, struct RIGpuProfiler_s *p, struct RICmd_s *cmd )
{
	if( !p->enabled || !cmd )
		return;
	if( arrlen( p->openStack ) == 0 )
		return;

	const uint32_t scopeIdx = arrpop( p->openStack );
	struct RIGpuProfilerSlot_s *s = &p->slots[p->activeSlot];
	struct RIGpuProfilerScope_s *scope = &s->scopes[scopeIdx];

	if( scope->beginIdx != RI_GPU_PROFILER_INVALID_QUERY && s->queryCount < RI_GPU_PROFILER_MAX_QUERIES ) {
		scope->endIdx = s->queryCount++;
#if ( DEVICE_IMPL_VULKAN )
		if( RIIsTargetSelected( RI_DEVICE_API_VK ) ) {
			vkCmdWriteTimestamp2( cmd->vk.cmd, RI_GPU_PROFILER_TIMESTAMP_STAGE, s->vk.pool, scope->endIdx );
		}
#endif
	}

#if ( DEVICE_IMPL_VULKAN )
	// Only close labels this profiler opened, so begin/end stay balanced even if the extension appeared
	// between the two calls or a scope was opened while it was unavailable.
	if( RIIsTargetSelected( RI_DEVICE_API_VK ) && p->openLabels > 0 && vkCmdEndDebugUtilsLabelEXT ) {
		vkCmdEndDebugUtilsLabelEXT( cmd->vk.cmd );
		p->openLabels--;
	}
#endif
}

void RIGpuProfilerResolve( struct RIDevice_s *dev, struct RIGpuProfiler_s *p, uint64_t completedTimeline )
{
	if( !p->enabled )
		return;

#if ( DEVICE_IMPL_VULKAN )
	if( RIIsTargetSelected( RI_DEVICE_API_VK ) ) {
		// Pick the newest finished slot; older finished ones are just marked resolved. Results from the
		// previous resolve stay in place when nothing new is ready, instead of flickering to empty.
		struct RIGpuProfilerSlot_s *newest = NULL;
		for( uint32_t i = 0; i < p->numSlots; i++ ) {
			struct RIGpuProfilerSlot_s *s = &p->slots[i];
			if( s->resolved || s->timelineValue > completedTimeline )
				continue;
			s->resolved = true;
			if( s->queryCount == 0 || s->timelineValue <= p->resultsTimeline )
				continue;
			if( !newest || s->timelineValue > newest->timelineValue )
				newest = s;
		}
		if( !newest )
			return;

		uint64_t timestamps[RI_GPU_PROFILER_MAX_QUERIES];
		VkResult vkResult = vkGetQueryPoolResults( dev->vk.device, newest->vk.pool, 0, newest->queryCount, newest->queryCount * sizeof( uint64_t ), timestamps,
												   sizeof( uint64_t ), VK_QUERY_RESULT_64_BIT );
		if( vkResult != VK_SUCCESS ) {
			// The timeline says the submit finished, so NOT_READY here means the data is unusable; drop it.
			if( vkResult != VK_NOT_READY )
				VK_WrapResult( vkResult );
			return;
		}

		arrsetlen( p->results, 0 );
		double depth0Total = 0;
		for( size_t j = 0; j < arrlen( newest->scopes ); j++ ) {
			const struct RIGpuProfilerScope_s *scope = &newest->scopes[j];
			struct RIGpuPassTiming_s timing = { .ms = 0, .depth = scope->depth };
			memcpy( timing.name, scope->name, sizeof( timing.name ) );
			if( scope->beginIdx != RI_GPU_PROFILER_INVALID_QUERY && scope->endIdx != RI_GPU_PROFILER_INVALID_QUERY ) {
				// masked difference handles counter wraparound within timestampValidBits
				const uint64_t ticks = ( timestamps[scope->endIdx] - timestamps[scope->beginIdx] ) & p->validBitsMask;
				timing.ms = (float)( (double)ticks * p->ticksToMs );
			}
			arrpush( p->results, timing );
			if( scope->depth == 0 )
				depth0Total += timing.ms;
		}
		p->totalMs = (float)depth0Total;
		p->resultsTimeline = newest->timelineValue;
	}
#endif
}
