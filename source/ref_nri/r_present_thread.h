#ifndef R_PRESENT_THREAD_H
#define R_PRESENT_THREAD_H

#include "ri_prelude.h"

// The presentation half of the frame, moved off the client's main thread.
//
// The main thread records the whole frame into rsh.backbuffer[frameIndex] and never touches the
// swapchain, so it never has to wait for the presentation engine. It submits its primary command
// buffer signalling PresentFrameDoneSemaphore(frameIndex), then hands the slot over here. This thread
// acquires an image (blocking on the vblank is free here), copies the backbuffer into it and presents.
//
// Slot lifetime: backbuffer[frameIndex] stays live until this thread's blit has retired, which
// RF_PresentSlotReady reports without blocking. The main thread uses that to decline a frame rather
// than stall, so declining a tick returns the client to input/network work instead of the vblank.

void RF_PresentThreadInit( void );
void RF_PresentThreadShutdown( void );

// Bind the thread to the queue it submits and presents on. Which queue that is only becomes known
// once a surface exists (InitRISwapchain verifies present support and may fall back), and the
// thread's command pools are tied to a queue family, so this cannot happen at init. RF_SetMode calls
// it right after InitRISwapchain; the thread is already drained at that point. Rebuilding the ring is
// skipped when the family is unchanged.
struct RIQueue_s;
void RF_PresentThreadBindQueue( struct RIQueue_s *queue );

// Hand a fully recorded and submitted frame over. Returns immediately. frameValue is the
// rsh.frameTimeline value the frame's primary submit signals -- the present thread's blit waits on it
// before reading backbuffer[frameIndex].
void RF_PresentThreadSubmit( uint32_t frameIndex, uint64_t frameValue );

// Block until every queued frame has been presented and its blit retired. Required before anything
// that tears down or resizes the swapchain, or destroys the device.
void RF_PresentThreadDrain( void );

// Non-blocking: has the present thread finished reading backbuffer[frameIndex]?
bool RF_PresentSlotReady( uint32_t frameIndex );

// Blocking form of the above, for the path that skipped the probe.
void RF_PresentSlotWait( uint32_t frameIndex );

// Serialises every submit across the main and present threads when they share a VkQueue --
// vkQueueSubmit and vkQueuePresentKHR are externally synchronised per queue. Both are no-ops once
// RF_PresentThreadBindQueue has resolved a dedicated present queue, which is the normal case.
void RF_QueueLock( void );
void RF_QueueUnlock( void );

#endif
