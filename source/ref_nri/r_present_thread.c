#include "r_local.h"
#include "r_present_thread.h"
#include "ri_renderer.h"
#include "ri_swapchain.h"

#include "tracy/TracyC.h"

enum { RPT_CMD_PRESENT = 0, RPT_CMD_SHUTDOWN = 1 };

struct rpt_presentCmd_s {
	int id;
	uint32_t frameIndex;
	uint64_t frameValue; // rsh.frameTimeline value the frame's primary submit signals
};

struct rpt_shutdownCmd_s {
	int id;
};

static struct {
	bool initialized;
	qthread_t *thread;
	qbufPipe_t *pipe;
	qmutex_t *queueLock;

	// Resolved by RF_PresentThreadBindQueue once the swapchain exists. When this is a distinct VkQueue
	// from every queue the main thread submits on, queueLock is dead weight and RF_QueueLock does
	// nothing -- that is the whole point of the split.
	struct RIQueue_s *queue;
	bool sharedQueue;
	bool ringBuilt;
	// expands to nothing when Tracy is compiled out
	TracyCLockCtx( tracyLock );

	// Thread-private command pools. Command pools are not thread safe, so this ring is only ever
	// touched from the present thread; its own sync primitives are unused because the fences below are
	// keyed by frame slot instead of by ring position.
	struct RICommandRingBuffer_s ring;

#if ( DEVICE_IMPL_VULKAN )
	VkFence presentFence[NUMBER_FRAMES_FLIGHT];
#endif
} rpt;

#if ( DEVICE_IMPL_VULKAN )
// Copy the frame's offscreen backbuffer into the acquired swapchain image and leave it in PRESENT_SRC.
// Formats and extents match by construction -- the backbuffer is created from rsh.swapchain.format at
// the swapchain dimensions -- so this is a plain copy rather than a blit.
static void __RPT_RecordBlit( struct RICmd_s *cmd, uint32_t frameIndex, uint32_t imageIndex )
{
	const VkImageSubresourceRange colorRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };

	{
		VkImageMemoryBarrier2 toDst = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		toDst.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		toDst.srcAccessMask = VK_ACCESS_2_NONE;
		toDst.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
		toDst.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		// nothing reads back the previous contents of a reacquired image
		toDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		toDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		toDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toDst.image = rsh.swapchain.vk.images[imageIndex];
		toDst.subresourceRange = colorRange;

		VkDependencyInfo dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &toDst;
		vkCmdPipelineBarrier2( cmd->vk.cmd, &dependencyInfo );
	}

	VkImageCopy region = { 0 };
	region.srcSubresource = ( VkImageSubresourceLayers ){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	region.dstSubresource = ( VkImageSubresourceLayers ){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	region.extent = ( VkExtent3D ){ rsh.swapchain.width, rsh.swapchain.height, 1 };
	vkCmdCopyImage( cmd->vk.cmd,
					rsh.backbuffer[frameIndex].vk.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					rsh.swapchain.vk.images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &region );

	{
		VkImageMemoryBarrier2 toPresent = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		toPresent.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
		toPresent.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		toPresent.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
		toPresent.dstAccessMask = VK_ACCESS_2_NONE;
		toPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toPresent.image = rsh.swapchain.vk.images[imageIndex];
		toPresent.subresourceRange = colorRange;

		VkDependencyInfo dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &toPresent;
		vkCmdPipelineBarrier2( cmd->vk.cmd, &dependencyInfo );
	}
}
#endif

// Runs on the present thread. Acquires an image for the frame recorded in backbuffer[frameIndex],
// blits it in and presents. The acquire is the vblank wait; parking here is the entire point.
static void __RPT_PresentFrame( uint32_t frameIndex, uint64_t frameValue )
{
#if ( DEVICE_IMPL_VULKAN )
	assert( frameIndex < NUMBER_FRAMES_FLIGHT );
	assert( rpt.ringBuilt );
	struct RIQueue_s *presentQueue = rpt.queue;

	// No fence wait here, and no reset: presentFence[frameIndex] is unsignalled before this job was
	// even queued (RF_EndFrame resets it), so waiting on it would block on this job's own fence.
	// Resetting the slot's pool is already safe -- RF_BeginFrame took RF_PresentSlotWait for this slot
	// before recording, which is exactly the fence of the previous job that used this pool.

	// Key the pool to the frame slot rather than free-running the ring: the fence that guards pool
	// reuse is presentFence[frameIndex], so the pool the fence protects has to be the pool we reset.
	rpt.ring.poolIndex = frameIndex;
	rpt.ring.cmdIndex = 0;
	rpt.ring.fenceIndex = 0;
	struct RICommandRingElement_s element = GetRICommandRingElement( &rsh.device, &rpt.ring, 1 );
	ResetRIPool( &rsh.device, element.pool );
	BeginRICmd( &rsh.device, &element.cmds[0] );

	uint32_t imageIndex = 0;
	{
		TracyCZoneN( acquire, "RPT_AcquireImage", 1 );
		imageIndex = RISwapchainAcquireNextTexture( &rsh.device, &rsh.swapchain );
		TracyCZoneEnd( acquire );
	}

	// OUT_OF_DATE: nothing was acquired and no semaphore was signalled. Still close and submit the
	// (empty) command buffer so presentFence[frameIndex] is signalled and the slot is released --
	// otherwise the main thread would wait on it forever. The rebuild happens on the main thread.
	const bool acquireFailed = rsh.swapchain.vk.acquireFailed;
	if( !acquireFailed ) {
		__RPT_RecordBlit( &element.cmds[0], frameIndex, imageIndex );
	}
	EndRICmd( &rsh.device, &element.cmds[0] );

	VkCommandBufferSubmitInfo cmdSubmitInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
	cmdSubmitInfo.commandBuffer = element.cmds[0].vk.cmd;

	VkSemaphoreSubmitInfo waitInfos[2];
	uint32_t waitCount = 0;
	// The frame's own rendering, which wrote the backbuffer this blit reads. This is the frame
	// timeline, not a per-slot binary semaphore: the wait crosses from the graphics queue to the
	// present queue, and a binary semaphore there has to be signalled exactly once per wait, with the
	// signal already submitted. Any hiccup in that pairing wedges the submit instead of erroring --
	// which is exactly what a dedicated present queue turned into a hang. A timeline value is
	// monotonic, safe to wait before it is signalled, and safe to wait on more than once.
	waitInfos[waitCount++] = ( VkSemaphoreSubmitInfo ){
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = rsh.frameTimeline.vk.semaphore,
		.value = frameValue,
		.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
	};
	if( !acquireFailed ) {
		waitInfos[waitCount++] = ( VkSemaphoreSubmitInfo ){
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = rsh.swapchain.vk.acquireSemaphores[rsh.swapchain.vk.acquireIdx],
			.stageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
		};
	}

	VkSemaphoreSubmitInfo signalInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = rsh.swapchain.vk.presentSemaphores[imageIndex],
		.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
	};

	VkSubmitInfo2 submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
	submitInfo.pCommandBufferInfos = &cmdSubmitInfo;
	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pWaitSemaphoreInfos = waitInfos;
	submitInfo.waitSemaphoreInfoCount = waitCount;
	submitInfo.pSignalSemaphoreInfos = &signalInfo;
	submitInfo.signalSemaphoreInfoCount = acquireFailed ? 0 : 1;

	RF_QueueLock();
	{
		TracyCZoneN( blitSubmit, "RPT_BlitSubmit", 1 );
		VK_WrapResult( vkQueueSubmit2( presentQueue->vk.queue, 1, &submitInfo, rpt.presentFence[frameIndex] ) );
		TracyCZoneEnd( blitSubmit );

		if( !acquireFailed ) {
			VkSemaphore presentWait[] = { rsh.swapchain.vk.presentSemaphores[imageIndex] };
			VkResult presentResult = RISwapchainPresent_vk( &rsh.device, &rsh.swapchain, imageIndex, 1, presentWait );
			if( presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR )
				rsh.swapchain.vk.outOfDate = 1;
			else if( presentResult != VK_SUCCESS )
				VK_WrapResult( presentResult );
		}
	}
	RF_QueueUnlock();

	rsh.swapchain.vk.acquireFailed = 0;
#endif
}

static unsigned __RPT_HandlePresent( const void *pcmd )
{
	const struct rpt_presentCmd_s *cmd = pcmd;
	__RPT_PresentFrame( cmd->frameIndex, cmd->frameValue );
	return sizeof( *cmd );
}

static unsigned __RPT_HandleShutdown( const void *pcmd )
{
	(void)pcmd;
	return 0; // terminates the pipe, which drops the thread out of BufPipe_Wait
}

static unsigned ( *rpt_cmdHandlers[] )( const void * ) = {
	__RPT_HandlePresent,
	__RPT_HandleShutdown,
};

static int __RPT_ReadCmds( qbufPipe_t *pipe, unsigned ( **handlers )( const void * ), bool onlyWake )
{
	(void)onlyWake;
	return ri.BufPipe_ReadCmds( pipe, handlers );
}

static void *__RPT_ThreadProc( void *param )
{
	(void)param;
	TracyCSetThreadName( "r_present" );
	ri.BufPipe_Wait( rpt.pipe, __RPT_ReadCmds, rpt_cmdHandlers, 100 );
	return NULL;
}

void RF_PresentThreadInit( void )
{
	assert( !rpt.initialized );
	memset( &rpt, 0, sizeof( rpt ) );

	rpt.queueLock = ri.Mutex_Create();
	TracyCLockAnnounce( rpt.tracyLock );
	TracyCLockCustomName( rpt.tracyLock, "RF_QueueLock", 12 );
	// The ring's command pools are bound to a queue family, and which family presents is not known
	// until a surface exists. RF_PresentThreadBindQueue builds the ring; RF_SetMode calls it.
	rpt.sharedQueue = true;

#if ( DEVICE_IMPL_VULKAN )
	for( uint32_t i = 0; i < NUMBER_FRAMES_FLIGHT; i++ ) {
		// created signalled: the first frame in each slot must not wait for a present that never happened
		VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, VK_FENCE_CREATE_SIGNALED_BIT };
		VK_WrapResult( vkCreateFence( rsh.device.vk.device, &fenceCreateInfo, NULL, &rpt.presentFence[i] ) );
	}
#endif

	rpt.pipe = ri.BufPipe_Create( sizeof( struct rpt_presentCmd_s ) * 64, 1 );
	rpt.thread = ri.Thread_Create( __RPT_ThreadProc, NULL );
	rpt.initialized = true;
}

void RF_PresentThreadBindQueue( struct RIQueue_s *queue )
{
	assert( rpt.initialized );
	assert( queue && queue->vk.queue );

	// Callers reach this from RF_SetMode, which has already drained the thread, so the ring is idle.
	if( rpt.ringBuilt ) {
		if( rpt.queue && rpt.queue->vk.queueFamilyIdx == queue->vk.queueFamilyIdx ) {
			rpt.queue = queue;
			return;
		}
		FreeRICommandRingBuffer( &rsh.device, &rpt.ring );
		rpt.ringBuilt = false;
	}

	rpt.queue = queue;
	InitRICommandRingBuffer( &rsh.device, queue, &rpt.ring, NUMBER_FRAMES_FLIGHT, 1, false );
	rpt.ringBuilt = true;

#if ( DEVICE_IMPL_VULKAN )
	// Every queue the main thread submits on. vkQueueSubmit/vkQueuePresentKHR are externally
	// synchronised per queue, so sharing any of these means the mutex has to stay live.
	rpt.sharedQueue = ( queue->vk.queue == rsh.device.queues[RI_QUEUE_GRAPHICS].vk.queue ) ||
					  ( queue->vk.queue == rsh.device.queues[RI_QUEUE_COPY].vk.queue );
#else
	rpt.sharedQueue = true;
#endif
	Com_Printf( "present thread: queue family %u slot %u, %s", queue->vk.queueFamilyIdx, queue->vk.slotIdx,
				rpt.sharedQueue ? "shared with the main thread (submits are serialised)" : "dedicated (no submit lock)" );
}

void RF_PresentThreadShutdown( void )
{
	if( !rpt.initialized )
		return;

	RF_PresentThreadDrain();

	struct rpt_shutdownCmd_s cmd = { RPT_CMD_SHUTDOWN };
	ri.BufPipe_WriteCmd( rpt.pipe, &cmd, sizeof( cmd ) );
	ri.Thread_Join( rpt.thread );
	ri.BufPipe_Destroy( &rpt.pipe );

#if ( DEVICE_IMPL_VULKAN )
	for( uint32_t i = 0; i < NUMBER_FRAMES_FLIGHT; i++ ) {
		if( rpt.presentFence[i] )
			vkDestroyFence( rsh.device.vk.device, rpt.presentFence[i], NULL );
	}
#endif

	if( rpt.ringBuilt )
		FreeRICommandRingBuffer( &rsh.device, &rpt.ring );
	TracyCLockTerminate( rpt.tracyLock );
	ri.Mutex_Destroy( &rpt.queueLock );
	memset( &rpt, 0, sizeof( rpt ) );
}

void RF_PresentThreadSubmit( uint32_t frameIndex, uint64_t frameValue )
{
	assert( rpt.initialized );
	assert( rpt.ringBuilt ); // RF_PresentThreadBindQueue has to have run; RF_SetMode does that

	// Mark the slot busy HERE, on the main thread, before the job is visible to the present thread.
	// Doing it over there instead left a window -- from this handover until the present thread got
	// past its blocking acquire -- in which the fence still read signalled from the slot's previous
	// job. The frame gate believed the slot was free, the main thread wrapped all NUMBER_FRAMES_FLIGHT
	// slots inside one vblank and re-signalled frameDone[frameIndex] while the queued job's wait was
	// still outstanding. Two signals, one wait: the pairing desyncs and both threads hang, the present
	// thread inside vkQueueSubmit2 and the main thread inside RF_PresentSlotWait.
#if ( DEVICE_IMPL_VULKAN )
	VK_WrapResult( vkResetFences( rsh.device.vk.device, 1, &rpt.presentFence[frameIndex] ) );
#endif

	struct rpt_presentCmd_s cmd = { RPT_CMD_PRESENT, frameIndex, frameValue };
	ri.BufPipe_WriteCmd( rpt.pipe, &cmd, sizeof( cmd ) );
}

void RF_PresentThreadDrain( void )
{
	if( !rpt.initialized )
		return;
	TracyCZoneN( drain, "RF_PresentThreadDrain", 1 );
	// empties the pipe...
	ri.BufPipe_Finish( rpt.pipe );
#if ( DEVICE_IMPL_VULKAN )
	// ...and then waits out the work the last job put on the queue.
	VK_WrapResult( vkWaitForFences( rsh.device.vk.device, NUMBER_FRAMES_FLIGHT, rpt.presentFence, VK_TRUE, UINT64_MAX ) );
#endif
	TracyCZoneEnd( drain );
}

bool RF_PresentSlotReady( uint32_t frameIndex )
{
	if( !rpt.initialized )
		return true;
#if ( DEVICE_IMPL_VULKAN )
	assert( frameIndex < NUMBER_FRAMES_FLIGHT );
	const VkResult status = vkGetFenceStatus( rsh.device.vk.device, rpt.presentFence[frameIndex] );
	if( status == VK_NOT_READY )
		return false;
	VK_WrapResult( status );
	return status == VK_SUCCESS;
#else
	return true;
#endif
}

void RF_PresentSlotWait( uint32_t frameIndex )
{
	if( !rpt.initialized )
		return;
#if ( DEVICE_IMPL_VULKAN )
	assert( frameIndex < NUMBER_FRAMES_FLIGHT );
	VK_WrapResult( vkWaitForFences( rsh.device.vk.device, 1, &rpt.presentFence[frameIndex], VK_TRUE, UINT64_MAX ) );
#endif
}

void RF_QueueLock( void )
{
	// No-op once the present thread owns its own VkQueue. On hardware that could not spare one this is
	// the only thing keeping two threads out of the same externally-synchronised queue, so the Tracy
	// annotation is here to make that contention visible rather than a mystery in a backtrace.
	if( !rpt.sharedQueue || !rpt.queueLock )
		return;
	TracyCLockBeforeLock( rpt.tracyLock );
	ri.Mutex_Lock( rpt.queueLock );
	TracyCLockAfterLock( rpt.tracyLock );
}

void RF_QueueUnlock( void )
{
	if( !rpt.sharedQueue || !rpt.queueLock )
		return;
	ri.Mutex_Unlock( rpt.queueLock );
	TracyCLockAfterUnlock( rpt.tracyLock );
}

