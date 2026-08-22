#include "ri_swapchain.h"
#include "qtypes.h"
#include "ri_format.h"
#include "ri_renderer.h"
#include "ri_timeline.h"
#include "ri_types.h"
#include "ri_vk.h"
#include "tracy/TracyC.h"
#if ( DEVICE_IMPL_VULKAN )

// TRANSFER_DST is what lets the present thread copy the offscreen backbuffer in. InitRISwapchain and
// RISwapchainResize both create images, and the resize path silently lacked TRANSFER_DST -- every
// present after an in-place rebuild was then an invalid vkCmdCopyImage. One definition, no drift.
#define RI_SWAPCHAIN_IMAGE_USAGE ( VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT )

static uint32_t __priority_BT709_G22_16BIT( const VkSurfaceFormatKHR *surface )
{
	return ( ( surface->format == VK_FORMAT_R16G16B16A16_SFLOAT ) << 0 ) | ( ( surface->colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT ) << 1 );
};

static uint32_t __priority_BT709_G22_8BIT( const VkSurfaceFormatKHR *surface )
{
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceFormatsKHR.html
	// There is always a corresponding UNORM, SRGB just need to consider UNORM
	return ( ( surface->format == VK_FORMAT_R8G8B8A8_UNORM || surface->format == VK_FORMAT_B8G8R8A8_UNORM ) << 0 ) | ( ( surface->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) << 1 );
}

static uint32_t __priority_BT709_G22_10BIT( const VkSurfaceFormatKHR *surface )
{
	return ( ( surface->format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 ) << 0 ) | ( ( surface->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) << 1 );
}

static uint32_t __priority_BT2020_G2084_10BIT( const VkSurfaceFormatKHR *surface )
{
	return ( ( surface->format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 ) << 0 ) | ( ( surface->colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT ) << 1 );
}

#endif

// Pick a concrete present mode from the surface's supported list. VK_PRESENT_MODE_FIFO_KHR must
// always be supported per spec, so it is the guaranteed fallback for both preferences. The vsync
// list omits IMMEDIATE so the caller can actually get blank-synced presentation; the low-latency
// list keeps the historical preference order.
static VkPresentModeKHR __RI_SelectPresentMode( uint8_t preference, const VkPresentModeKHR *supported, uint32_t supportedCount )
{
	static const VkPresentModeKHR lowLatencyModeList[] = { VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_FIFO_KHR };
	static const VkPresentModeKHR vsyncModeList[] = { VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_FIFO_KHR };

	const bool preferVsync = ( preference == RI_PRESENT_PREFER_VSYNC );
	const VkPresentModeKHR *preferredModeList = preferVsync ? vsyncModeList : lowLatencyModeList;
	const size_t preferredModeCount = preferVsync ? Q_ARRAY_COUNT( vsyncModeList ) : Q_ARRAY_COUNT( lowLatencyModeList );

	for( size_t j = 0; j < preferredModeCount; j++ ) {
		for( uint32_t i = 0; i < supportedCount; i++ ) {
			if( supported[i] == preferredModeList[j] )
				return preferredModeList[j];
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

int InitRISwapchain( struct RIDevice_s *dev, struct RISwapchainDesc_s *init, struct RISwapchain_s *swapchain )
{
	assert( init->windowHandle );
	assert( init );
	assert( swapchain );
	assert( init->requestImageCount <= Q_ARRAY_COUNT( swapchain->vk.images ) && init->requestImageCount > 0 );
	swapchain->width = init->width;
	swapchain->height = init->height;
	swapchain->presentQueue = init->queue;
	VkResult result = VK_SUCCESS;
#if ( DEVICE_IMPL_VULKAN )
	{
		switch( init->windowHandle->type ) {
#ifdef VK_USE_PLATFORM_XLIB_KHR
			case RI_WINDOW_X11: {
				VkXlibSurfaceCreateInfoKHR xlibSurfaceInfo = { VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR };
				xlibSurfaceInfo.dpy = init->windowHandle->x11.dpy;
				xlibSurfaceInfo.window = init->windowHandle->x11.window;
				result = vkCreateXlibSurfaceKHR( dev->renderer->vk.instance, &xlibSurfaceInfo, NULL, &swapchain->vk.surface );
				VK_WrapResult( result );
				break;
			}
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
			case RI_WINDOW_WIN32: {
				VkWin32SurfaceCreateInfoKHR win32SurfaceInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
				win32SurfaceInfo.hwnd = (HWND)init->windowHandle->windows.hwnd;

				result = vkCreateWin32SurfaceKHR( dev->renderer->vk.instance, &win32SurfaceInfo, NULL, &swapchain->vk.surface );
				VK_WrapResult( result );
				break;
			}
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
			case RI_WINDOW_METAL: {
				VkMetalSurfaceCreateInfoEXT metalSurfaceCreateInfo = { VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT };
				metalSurfaceCreateInfo.pLayer = (CAMetalLayer *)swapChainDesc.window.metal.caMetalLayer;

				VkResult result = vk.CreateMetalSurfaceEXT( m_Device, &metalSurfaceCreateInfo, m_Device.GetAllocationCallbacks(), &m_Surface );
				RETURN_ON_FAILURE( &m_Device, result == VK_SUCCESS, GetReturnCode( result ), "vkCreateMetalSurfaceEXT returned %d", (int32_t)result );
				break;
			}
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
			case RI_WINDOW_WAYLAND: {
				VkWaylandSurfaceCreateInfoKHR waylandSurfaceInfo = { VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR };
				waylandSurfaceInfo.display = init->windowHandle->wayland.display;
				waylandSurfaceInfo.surface = init->windowHandle->wayland.surface;
				result = vkCreateWaylandSurfaceKHR( dev->renderer->vk.instance, &waylandSurfaceInfo, NULL, &swapchain->vk.surface );
				VK_WrapResult( result );
				break;
			}
#endif
			default:
				break;
		}
	}
#endif
	{
		// Queue selection happens at device creation, before any surface exists, so this is the first
		// point at which the requested present queue's family can actually be checked against the
		// surface. Fall back to graphics rather than fail: a shared queue still presents correctly, it
		// just puts RF_QueueLock back in the path.
		VkBool32 presentSupported = VK_FALSE;
		VK_WrapResult( vkGetPhysicalDeviceSurfaceSupportKHR( dev->physicalAdapter.vk.physicalDevice, swapchain->presentQueue->vk.queueFamilyIdx,
															 swapchain->vk.surface, &presentSupported ) );
		if( !presentSupported ) {
			Com_Printf( "VK swapchain: queue family %u cannot present to this surface, falling back to the graphics queue",
						swapchain->presentQueue->vk.queueFamilyIdx );
			swapchain->presentQueue = &dev->queues[RI_QUEUE_GRAPHICS];
		}
		Com_Printf( "VK swapchain: presenting on queue family %u slot %u%s", swapchain->presentQueue->vk.queueFamilyIdx,
					swapchain->presentQueue->vk.slotIdx,
					( swapchain->presentQueue->vk.queue == dev->queues[RI_QUEUE_GRAPHICS].vk.queue ) ? " (shared with graphics)" : " (dedicated)" );
	}

	VkSurfaceCapabilitiesKHR surfaceCaps = { 0 };
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR( dev->physicalAdapter.vk.physicalDevice, swapchain->vk.surface, &surfaceCaps );
	VK_WrapResult( result );

	uint32_t numSurfaceFormats = 0;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR( dev->physicalAdapter.vk.physicalDevice, swapchain->vk.surface, &numSurfaceFormats, NULL );
	VK_WrapResult( result );
	VkSurfaceFormatKHR *surfaceFormats = malloc( sizeof( VkSurfaceFormatKHR ) * numSurfaceFormats );
	result = vkGetPhysicalDeviceSurfaceFormatsKHR( dev->physicalAdapter.vk.physicalDevice, swapchain->vk.surface, &numSurfaceFormats, surfaceFormats );
	VK_WrapResult( result );
	VkSurfaceFormatKHR *selectedSurf = surfaceFormats;
	{
		uint32_t ( *priorityHandler )( const VkSurfaceFormatKHR *surface ) = __priority_BT709_G22_8BIT;
		switch( init->format ) {
			case RI_SWAPCHAIN_BT709_G10_16BIT:
				priorityHandler = __priority_BT709_G22_16BIT;
				break;
			case RI_SWAPCHAIN_BT709_G22_8BIT:
				priorityHandler = __priority_BT709_G22_8BIT;
				break;
			case RI_SWAPCHAIN_BT709_G22_10BIT:
				priorityHandler = __priority_BT709_G22_10BIT;
				break;
			case RI_SWAPCHAIN_BT2020_G2084_10BIT:
				priorityHandler = __priority_BT2020_G2084_10BIT;
				break;
		}
		for( size_t i = 1; i < numSurfaceFormats; i++ ) {
			assert( priorityHandler );
			if( priorityHandler( surfaceFormats + i ) > priorityHandler( selectedSurf ) ) {
				selectedSurf = surfaceFormats + i;
			}
		}
	}

	uint32_t presentModeCount = 0;
	result = vkGetPhysicalDeviceSurfacePresentModesKHR( dev->physicalAdapter.vk.physicalDevice, swapchain->vk.surface, &presentModeCount, NULL );
	VK_WrapResult( result );
	VkPresentModeKHR *supportedPresentMode = malloc( presentModeCount * sizeof( VkPresentModeKHR ) );
	result = vkGetPhysicalDeviceSurfacePresentModesKHR( dev->physicalAdapter.vk.physicalDevice, swapchain->vk.surface, &presentModeCount, supportedPresentMode );
	VK_WrapResult( result );

	const VkPresentModeKHR presentMode = __RI_SelectPresentMode( init->presentPreference, supportedPresentMode, presentModeCount );
	{
		VkSwapchainCreateInfoKHR swapChainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		swapChainCreateInfo.flags = 0;
		swapChainCreateInfo.surface = swapchain->vk.surface;
		// clamp the requested image count to the surface capabilities. A surfaceCaps value of 0 means "unspecified" — skip that side of the clamp.
		uint32_t desiredImageCount = init->requestImageCount;
		if( surfaceCaps.minImageCount > 0 && desiredImageCount < surfaceCaps.minImageCount )
			desiredImageCount = surfaceCaps.minImageCount;
		if( surfaceCaps.maxImageCount > 0 && desiredImageCount > surfaceCaps.maxImageCount )
			desiredImageCount = surfaceCaps.maxImageCount;
		swapChainCreateInfo.minImageCount = desiredImageCount;
		swapChainCreateInfo.imageFormat = selectedSurf->format;
		swapChainCreateInfo.imageColorSpace = selectedSurf->colorSpace;
		swapChainCreateInfo.imageExtent.width = init->width;
		swapChainCreateInfo.imageExtent.height = init->height;
		swapChainCreateInfo.imageArrayLayers = 1;
		swapChainCreateInfo.imageUsage = RI_SWAPCHAIN_IMAGE_USAGE;
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = NULL;
		swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainCreateInfo.presentMode = presentMode;
		swapChainCreateInfo.clipped = VK_TRUE;
		swapChainCreateInfo.oldSwapchain = 0;
		result = vkCreateSwapchainKHR( dev->vk.device, &swapChainCreateInfo, NULL, &swapchain->vk.swapchain );
	}

	{
		uint32_t imageNum = 0;
		vkGetSwapchainImagesKHR( dev->vk.device, swapchain->vk.swapchain, &imageNum, NULL );
		assert( imageNum <= RI_MAX_SWAPCHAIN_IMAGES );
		vkGetSwapchainImagesKHR( dev->vk.device, swapchain->vk.swapchain, &imageNum, swapchain->vk.images );
		swapchain->vk.imageCount = imageNum;

		swapchain->format = VKToRIFormat( selectedSurf->format );
		swapchain->vk.imageColorSpace = selectedSurf->colorSpace;
		swapchain->vk.presentMode = presentMode;

		for( size_t i = 0; i < imageNum; i++ ) {
			VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
			VkSemaphoreTypeCreateInfo timelineCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
			timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_BINARY;
			R_VK_ADD_STRUCT( &createInfo, &timelineCreateInfo );

			result = vkCreateSemaphore( dev->vk.device, &createInfo, NULL, &swapchain->vk.acquireSemaphores[i] );
			VK_WrapResult( result );
			result = vkCreateSemaphore( dev->vk.device, &createInfo, NULL, &swapchain->vk.presentSemaphores[i] );
			VK_WrapResult( result );
		}

		for( size_t i = 0; i < imageNum; i++ ) {
			VkImageViewCreateInfo viewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			viewCreateInfo.image = swapchain->vk.images[i];
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCreateInfo.format = selectedSurf->format;
			viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewCreateInfo.subresourceRange.baseMipLevel = 0;
			viewCreateInfo.subresourceRange.levelCount = 1;
			viewCreateInfo.subresourceRange.baseArrayLayer = 0;
			viewCreateInfo.subresourceRange.layerCount = 1;
			result = vkCreateImageView( dev->vk.device, &viewCreateInfo, NULL, &swapchain->vk.views[i] );
			VK_WrapResult( result );
		}
	}
	free( supportedPresentMode );
	free( surfaceFormats );
	return RI_SUCCESS;
}

uint32_t RISwapchainAcquireNextTexture( struct RIDevice_s *dev, struct RISwapchain_s *swapchain )
{
	assert(swapchain->vk.imageCount > 0);
#if ( DEVICE_IMPL_VULKAN )
	{
		uint32_t image_index = 0;
		swapchain->vk.acquireIdx = ( swapchain->vk.acquireIdx + 1 ) % swapchain->vk.imageCount;
		VkSemaphore imageAcquiredSemaphore = swapchain->vk.acquireSemaphores[swapchain->vk.acquireIdx];
		VkResult result = vkAcquireNextImageKHR( dev->vk.device, swapchain->vk.swapchain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &image_index );
		switch( result ) {
			case VK_SUCCESS:
				break;
			case VK_SUBOPTIMAL_KHR:
				// The acquire still succeeded and the semaphore was signalled; flag the swapchain for
				// rebuild but render this frame normally.
				swapchain->vk.outOfDate = 1;
				break;
			case VK_ERROR_OUT_OF_DATE_KHR:
				// No image was acquired and the semaphore was not signalled. Mark the frame's acquire
				// as failed so the submit skips the acquire wait / present, and flag for rebuild.
				swapchain->vk.outOfDate = 1;
				swapchain->vk.acquireFailed = 1;
				break;
			default:
				// Nothing was acquired and no semaphore was signalled (SURFACE_LOST, DEVICE_LOST,
				// TIMEOUT, NOT_READY...). Falling through without acquireFailed would leave the blit
				// submit waiting on a semaphore nobody signals, which deadlocks the present thread and
				// then the main thread behind the slot fence.
				VK_WrapResult( result );
				swapchain->vk.outOfDate = 1;
				swapchain->vk.acquireFailed = 1;
				break;
		}
		return image_index;
	}
#endif
	return 0;
}

void FreeRISwapchain( struct RIDevice_s *dev, struct RISwapchain_s *swapchain )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		for( size_t p = 0; p < RI_MAX_SWAPCHAIN_IMAGES; p++ ) {
			if( swapchain->vk.acquireSemaphores[p] )
				vkDestroySemaphore( dev->vk.device, swapchain->vk.acquireSemaphores[p], NULL );
			if( swapchain->vk.presentSemaphores[p] )
				vkDestroySemaphore( dev->vk.device, swapchain->vk.presentSemaphores[p], NULL );
		}
		for( size_t p = 0; p < RI_MAX_SWAPCHAIN_IMAGES; p++ ) {
			if( swapchain->vk.views[p] )
				vkDestroyImageView( dev->vk.device, swapchain->vk.views[p], NULL );
		}
		if( swapchain->vk.swapchain )
			vkDestroySwapchainKHR( dev->vk.device, swapchain->vk.swapchain, NULL );
		if( swapchain->vk.surface )
			vkDestroySurfaceKHR( dev->renderer->vk.instance, swapchain->vk.surface, NULL );
	}
	memset( swapchain, 0, sizeof( struct RISwapchain_s ) );
#endif
}

VkResult RISwapchainPresent_vk( struct RIDevice_s *dev, struct RISwapchain_s *swapchain, uint32_t index, size_t num_wait_semaphores, VkSemaphore *wait_semaphores )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.waitSemaphoreCount = num_wait_semaphores;
		presentInfo.pWaitSemaphores = wait_semaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain->vk.swapchain;
		presentInfo.pImageIndices = &index;
		{
			// CPU back-pressure point: the WSI can block here when every image is still queued.
			TracyCZoneN( present, "vkQueuePresentKHR", 1 );
			const VkResult presentResult = vkQueuePresentKHR( swapchain->presentQueue->vk.queue, &presentInfo );
			TracyCZoneEnd( present );
			return presentResult;
		}
	}
#endif
	return VK_SUCCESS;
}

int RIQueueFrameSubmit( struct RIDevice_s *dev, struct RIQueue_s *queue, struct RIQueueFrameSubmitDesc_s *desc )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		VkCommandBufferSubmitInfo cmdSubmitInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
		cmdSubmitInfo.commandBuffer = desc->cmd->vk.cmd;

		const size_t numSignal = desc->vk.numSignalSemaphores + ( desc->timeline ? 1 : 0 );
		VkSemaphoreSubmitInfo *signalInfos = alloca( sizeof( VkSemaphoreSubmitInfo ) * ( numSignal > 0 ? numSignal : 1 ) );
		size_t signalCount = 0;
		for( size_t i = 0; i < desc->vk.numSignalSemaphores; i++ )
			signalInfos[signalCount++] = desc->vk.signalSemaphores[i];

		// The frame timeline paces resource reclaim and GPU query readback against the submit, not the
		// present. (value is ignored for binary semaphores.)
		if( desc->timeline ) {
			signalInfos[signalCount++] = ( VkSemaphoreSubmitInfo ){
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = desc->timeline->vk.semaphore,
				.value = RITimelineNext( desc->timeline ),
				.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
			};
		}

		VkSubmitInfo2 submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
		submitInfo.pCommandBufferInfos = &cmdSubmitInfo;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pWaitSemaphoreInfos = desc->vk.waitSemaphores;
		submitInfo.waitSemaphoreInfoCount = desc->vk.numWaitSemaphores;
		submitInfo.pSignalSemaphoreInfos = signalInfos;
		submitInfo.signalSemaphoreInfoCount = signalCount;

		assert( vkGetFenceStatus( dev->vk.device, desc->ringElement->vk.fence ) == VK_SUCCESS );
		VK_WrapResult( vkResetFences( dev->vk.device, 1, &desc->ringElement->vk.fence ) );
		{
			TracyCZoneN( primarySubmit, "vkQueueSubmit2", 1 );
			VK_WrapResult( vkQueueSubmit2( queue->vk.queue, 1, &submitInfo, desc->ringElement->vk.fence ) );
			TracyCZoneEnd( primarySubmit );
		}
	}
#endif
	return RI_SUCCESS;
}

bool RISwapchainSetPresentPreference( struct RIDevice_s *dev, struct RISwapchain_s *swapchain, uint8_t preference )
{
#if ( DEVICE_IMPL_VULKAN )
	uint32_t presentModeCount = 0;
	VK_WrapResult( vkGetPhysicalDeviceSurfacePresentModesKHR( dev->physicalAdapter.vk.physicalDevice, swapchain->vk.surface, &presentModeCount, NULL ) );
	if( presentModeCount == 0 )
		return false;

	VkPresentModeKHR *supportedPresentMode = malloc( presentModeCount * sizeof( VkPresentModeKHR ) );
	VK_WrapResult( vkGetPhysicalDeviceSurfacePresentModesKHR( dev->physicalAdapter.vk.physicalDevice, swapchain->vk.surface, &presentModeCount, supportedPresentMode ) );

	const VkPresentModeKHR mode = __RI_SelectPresentMode( preference, supportedPresentMode, presentModeCount );
	free( supportedPresentMode );

	if( mode == swapchain->vk.presentMode )
		return false;

	// The mode is baked into the VkSwapchainKHR, so flag a rebuild. RISwapchainResize reads
	// swapchain->vk.presentMode and honors outOfDate even when the extent is unchanged.
	swapchain->vk.presentMode = mode;
	swapchain->vk.outOfDate = 1;
	return true;
#else
	return false;
#endif
}

int RISwapchainResize( struct RIDevice_s *dev, struct RISwapchain_s *swapchain, uint16_t width, uint16_t height )
{
	// Nothing to do when the size is unchanged and the swapchain is still valid. An OUT_OF_DATE
	// swapchain must be rebuilt even at the same size, so honor that flag here.
	if( width == swapchain->width && height == swapchain->height && !swapchain->vk.outOfDate )
		return 0;
#if ( DEVICE_IMPL_VULKAN )
	{
		VkResult result;
		VkSwapchainKHR oldSwapchain = swapchain->vk.swapchain;

		VkSwapchainCreateInfoKHR swapChainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		swapChainCreateInfo.surface = swapchain->vk.surface;
		swapChainCreateInfo.minImageCount = swapchain->vk.imageCount;
		swapChainCreateInfo.imageFormat = RIFormatToVK( swapchain->format );
		swapChainCreateInfo.imageColorSpace = swapchain->vk.imageColorSpace;
		swapChainCreateInfo.imageExtent.width = width;
		swapChainCreateInfo.imageExtent.height = height;
		swapChainCreateInfo.imageArrayLayers = 1;
		swapChainCreateInfo.imageUsage = RI_SWAPCHAIN_IMAGE_USAGE;
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainCreateInfo.presentMode = swapchain->vk.presentMode;
		swapChainCreateInfo.clipped = VK_TRUE;
		swapChainCreateInfo.oldSwapchain = oldSwapchain;

		// Create the replacement into a temporary handle first so that, on failure, all existing
		// handles remain intact and the caller can fall back to a full teardown + re-init.
		VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
		result = vkCreateSwapchainKHR( dev->vk.device, &swapChainCreateInfo, NULL, &newSwapchain );
		if( result != VK_SUCCESS ) {
			VK_WrapResult( result );
			return RI_FAIL;
		}
		swapchain->vk.swapchain = newSwapchain;

		vkDestroySwapchainKHR( dev->vk.device, oldSwapchain, NULL );

		for( size_t i = 0; i < swapchain->vk.imageCount; i++ ) {
			if( swapchain->vk.views[i] )
				vkDestroyImageView( dev->vk.device, swapchain->vk.views[i], NULL );
			swapchain->vk.views[i] = VK_NULL_HANDLE;
		}

		uint32_t imageNum = 0;
		vkGetSwapchainImagesKHR( dev->vk.device, swapchain->vk.swapchain, &imageNum, NULL );
		assert( imageNum <= RI_MAX_SWAPCHAIN_IMAGES );
		vkGetSwapchainImagesKHR( dev->vk.device, swapchain->vk.swapchain, &imageNum, swapchain->vk.images );

		for( size_t i = 0; i < imageNum; i++ ) {
			VkImageViewCreateInfo viewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			viewCreateInfo.image = swapchain->vk.images[i];
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCreateInfo.format = RIFormatToVK( swapchain->format );
			viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewCreateInfo.subresourceRange.baseMipLevel = 0;
			viewCreateInfo.subresourceRange.levelCount = 1;
			viewCreateInfo.subresourceRange.baseArrayLayer = 0;
			viewCreateInfo.subresourceRange.layerCount = 1;
			result = vkCreateImageView( dev->vk.device, &viewCreateInfo, NULL, &swapchain->vk.views[i] );
			VK_WrapResult( result );
		}

		// The per-image semaphores must be recreated: imageCount can change across a resize, and a
		// semaphore left pending by an acquire or a present on the retired swapchain is unusable.
		// Reset the round-robin index so it stays in range. (rhi-zig's swapchain.zig resize omits
		// this — a bug.) This runs after vkDestroySwapchainKHR( oldSwapchain ) above, which aborts
		// any presents still holding a wait on the present semaphores.
		for( size_t i = 0; i < RI_MAX_SWAPCHAIN_IMAGES; i++ ) {
			if( swapchain->vk.acquireSemaphores[i] ) {
				vkDestroySemaphore( dev->vk.device, swapchain->vk.acquireSemaphores[i], NULL );
				swapchain->vk.acquireSemaphores[i] = VK_NULL_HANDLE;
			}
			if( swapchain->vk.presentSemaphores[i] ) {
				vkDestroySemaphore( dev->vk.device, swapchain->vk.presentSemaphores[i], NULL );
				swapchain->vk.presentSemaphores[i] = VK_NULL_HANDLE;
			}
		}
		for( size_t i = 0; i < imageNum; i++ ) {
			VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
			VkSemaphoreTypeCreateInfo binaryCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
			binaryCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_BINARY;
			R_VK_ADD_STRUCT( &createInfo, &binaryCreateInfo );
			result = vkCreateSemaphore( dev->vk.device, &createInfo, NULL, &swapchain->vk.acquireSemaphores[i] );
			VK_WrapResult( result );
			result = vkCreateSemaphore( dev->vk.device, &createInfo, NULL, &swapchain->vk.presentSemaphores[i] );
			VK_WrapResult( result );
		}

		swapchain->vk.imageCount = imageNum;
		swapchain->vk.acquireIdx = 0;
		swapchain->vk.outOfDate = 0;
		swapchain->vk.acquireFailed = 0;
		swapchain->width = width;
		swapchain->height = height;
	}
#endif
	return 1;
}

struct RITextureView_s RISwapchainGetTextureView(struct RISwapchain_s *swapchain, uint32_t index) {
	struct RITextureView_s view = {
		.vk = {
			.image = swapchain->vk.views[index]
		}
	};
	return view;
}

struct RITexture_s RISwapchainGetTexture(struct RISwapchain_s *swapchain, uint32_t index) {
	struct RITexture_s texture = {
		.vk = {
			.image = swapchain->vk.images[index],
			.allocation = NULL // swapchain-owned; not a VMA allocation
		}
	};
	return texture;
}

uint32_t RISwapchainGetImageCount( struct RISwapchain_s *swapchain )
{
#if ( DEVICE_IMPL_VULKAN )
	return swapchain->vk.imageCount;
#endif
	return 0;
}
