#ifndef RI_TIMELINE_H
#define RI_TIMELINE_H

#include "ri_types.h"

// Thin wrapper over a Vulkan timeline semaphore with a CPU-side mirror of the last value handed to a
// submit. The model is monotonic reserve/poll: RITimelineNext reserves the value for the next submit,
// RITimelineCompleted polls (non-blocking) how far the GPU has progressed, and resource reclaim /
// query readback keys off that. Ported from rhi-zig timeline.zig.
//
// Invariant: exactly one producer calls RITimelineNext (the frame submit), so RITimelinePending equals
// the value the in-flight frame's submit will (or did) signal.

int InitRITimeline( struct RIDevice_s *dev, struct RITimeline_s *timeline );
void FreeRITimeline( struct RIDevice_s *dev, struct RITimeline_s *timeline );

// Reserve the next signal value. Call immediately before the submit that signals it.
uint64_t RITimelineNext( struct RITimeline_s *timeline );
// Highest value handed out by RITimelineNext (the latest in-flight submit).
uint64_t RITimelinePending( const struct RITimeline_s *timeline );
// Non-blocking poll of how far the GPU has actually progressed. Poll this in the frame loop.
uint64_t RITimelineCompleted( struct RIDevice_s *dev, struct RITimeline_s *timeline );
// Block until the GPU reaches value. Shutdown only; prefer polling RITimelineCompleted in the loop.
void RITimelineWait( struct RIDevice_s *dev, struct RITimeline_s *timeline, uint64_t value );

#endif
