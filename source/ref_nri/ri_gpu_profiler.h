#ifndef RI_GPU_PROFILER_H
#define RI_GPU_PROFILER_H

#include "ri_types.h"

// Timestamp-query GPU profiler ported from rhi-zig gpu_profiler.zig. Each frame-in-flight slot owns a
// timestamp query pool; named scopes (nestable, with depth) write a pair of timestamps. Readback is
// timeline-gated and non-blocking: a slot is only resolved once its submit's timeline value has been
// reached, so the frame loop never stalls on the GPU. Results lag by the frames-in-flight depth.

#define RI_GPU_PROFILER_MAX_QUERIES 256   // timestamps per slot (128 scope pairs)
#define RI_GPU_PROFILER_MAX_SLOTS 4       // >= NUMBER_FRAMES_FLIGHT; runtime count passed to Init
#define RI_GPU_PROFILER_INVALID_QUERY 0xffffffffu

struct RIGpuPassTiming_s {
	const char *name; // borrowed; scope names must be string literals with static lifetime
	float ms;
	uint32_t depth;
};

struct RIGpuProfilerScope_s {
	const char *name;
	uint32_t depth;
	uint32_t beginIdx; // RI_GPU_PROFILER_INVALID_QUERY when the per-frame query budget was exhausted
	uint32_t endIdx;
};

struct RIGpuProfilerSlot_s {
	uint64_t timelineValue;                // frame timeline value this slot's submit will signal
	bool resolved;                         // results already read back
	uint32_t queryCount;                   // timestamps written this frame
	struct RIGpuProfilerScope_s *scopes;   // stb_ds array
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkQueryPool pool;
		} vk;
#endif
	};
};

struct RIGpuProfiler_s {
	struct RIGpuProfilerSlot_s slots[RI_GPU_PROFILER_MAX_SLOTS];
	uint32_t numSlots;
	uint32_t *openStack;                   // stb_ds array of open scope indices
	uint32_t activeSlot;
	bool enabled;
	double ticksToMs;
	uint64_t validBitsMask;
	struct RIGpuPassTiming_s *results;     // stb_ds array; valid until the next Resolve
	float totalMs;                         // sum of depth-0 scopes from the last resolved slot
};

void InitRIGpuProfiler( struct RIDevice_s *dev, uint32_t numSlots, struct RIGpuProfiler_s *profiler );
void FreeRIGpuProfiler( struct RIDevice_s *dev, struct RIGpuProfiler_s *profiler );

// Call after the primary command buffer begin() and before any render pass (vkCmdResetQueryPool is
// illegal inside dynamic rendering). slot is the current frame-in-flight index; timelineValue is what
// this frame's submit will signal.
void RIGpuProfilerBeginFrame( struct RIDevice_s *dev, struct RIGpuProfiler_s *p, struct RICmd_s *cmd, uint32_t slot, uint64_t timelineValue );
// name must be a string literal (borrowed until the next BeginFrame of this slot).
void RIGpuProfilerBeginScope( struct RIDevice_s *dev, struct RIGpuProfiler_s *p, struct RICmd_s *cmd, const char *name );
void RIGpuProfilerEndScope( struct RIDevice_s *dev, struct RIGpuProfiler_s *p, struct RICmd_s *cmd );
// Read back any one slot the GPU has finished. Non-blocking; call once per frame with the last
// completed timeline value.
void RIGpuProfilerResolve( struct RIDevice_s *dev, struct RIGpuProfiler_s *p, uint64_t completedTimeline );

#endif
