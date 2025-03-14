#include "ri_swapchain.h"
#include "ri_renderer.h"
#include "ri_types.h"
#include "ri_format.h"
#include <vulkan/vulkan_core.h>


#if ( DEVICE_IMPL_VULKAN )

static uint32_t __priority_BT709_G22_16BIT(const VkSurfaceFormatKHR* surface)  {
    return ((surface->format == VK_FORMAT_R16G16B16A16_SFLOAT) << 0) | 
           ((surface->colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) << 1);
};

static uint32_t __priority_BT709_G22_8BIT(const VkSurfaceFormatKHR* surface) {
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceFormatsKHR.html
    // There is always a corresponding UNORM, SRGB just need to consider UNORM
    return ((surface->format == VK_FORMAT_R8G8B8A8_UNORM || surface->format == VK_FORMAT_B8G8R8A8_UNORM) << 0) | 
  				 ((surface->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) << 1);
}

static uint32_t __priority_BT709_G22_10BIT(const VkSurfaceFormatKHR* surface){
    return ((surface->format == VK_FORMAT_A2B10G10R10_UNORM_PACK32) << 0) | 
           ((surface->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) << 1);
}

static uint32_t __priority_BT2020_G2084_10BIT( const VkSurfaceFormatKHR *surface )
{
	return ( ( surface->format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 ) << 0 ) | ( ( surface->colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT ) << 1 );
}

#endif

int InitRISwapchain( struct RIDevice_s *dev, struct RISwapchainDesc_s *init, struct RISwapchain_s *swapchain )
{
	assert( init->windowHandle );
	assert( init );
	assert( swapchain );
	assert( init->imageCount <= Q_ARRAY_COUNT( swapchain->vk.images ) && init->imageCount > 0 );
	swapchain->width = init->width;
	swapchain->height = init->height;
	swapchain->presentQueue = init->queue;
	swapchain->imageCount = init->imageCount;
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
				// RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateXlibSurfaceKHR returned %d", (int32_t)result);
				break;
			}
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
			case RI_WINDOW_WIN32: {
				VkWin32SurfaceCreateInfoKHR win32SurfaceInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
				win32SurfaceInfo.hwnd = (HWND)swapChainDesc.window.windows.hwnd;

				VkResult result = vk.CreateWin32SurfaceKHR( m_Device, &win32SurfaceInfo, m_Device.GetAllocationCallbacks(), &m_Surface );
				RETURN_ON_FAILURE( &m_Device, result == VK_SUCCESS, GetReturnCode( result ), "vkCreateWin32SurfaceKHR returned %d", (int32_t)result );
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
		VkBool32 supported = VK_FALSE;
		result = vkGetPhysicalDeviceSurfaceSupportKHR(dev->physicalAdapter.vk.physicalDevice, init->queue->vk.queueFamilyIdx, swapchain->vk.surface, &supported);
		VK_WrapResult(result);

		VkSurfaceCapabilitiesKHR surfaceCaps = {};
		result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev->physicalAdapter.vk.physicalDevice, swapchain->vk.surface, &surfaceCaps);
		VK_WrapResult(result);
    VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
    surfaceInfo.surface = swapchain->vk.surface;
		
		result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev->physicalAdapter.vk.physicalDevice, swapchain->vk.surface, &surfaceCaps);
		VK_WrapResult(result);
	}
	
	uint32_t numSurfaceFormats = 0;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR( dev->physicalAdapter.vk.physicalDevice, swapchain->vk.surface, &numSurfaceFormats, NULL );
	VK_WrapResult(result);
	VkSurfaceFormatKHR *surfaceFormats = malloc( sizeof( VkSurfaceFormatKHR ) * numSurfaceFormats );
	result = vkGetPhysicalDeviceSurfaceFormatsKHR( dev->physicalAdapter.vk.physicalDevice, swapchain->vk.surface, &numSurfaceFormats, surfaceFormats );
	VK_WrapResult(result);
	VkSurfaceFormatKHR *selectedSurf = surfaceFormats ;
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
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(dev->physicalAdapter.vk.physicalDevice, swapchain->vk.surface, &presentModeCount, NULL);
	VK_WrapResult(result);
	VkPresentModeKHR* supportedPresentMode = malloc(presentModeCount * sizeof(VkPresentModeKHR));
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(dev->physicalAdapter.vk.physicalDevice, swapchain->vk.surface, &presentModeCount, supportedPresentMode);
	VK_WrapResult(result);

  // The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
  // This mode waits for the vertical blank ("v-sync")
  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

  VkPresentModeKHR preferredModeList[] = {
      VK_PRESENT_MODE_IMMEDIATE_KHR,
      VK_PRESENT_MODE_FIFO_RELAXED_KHR,
      VK_PRESENT_MODE_FIFO_KHR
  };
  for( size_t j = 0; j < Q_ARRAY_COUNT( preferredModeList ); j++ ) {
	  VkPresentModeKHR mode = preferredModeList[j];
	  uint32_t i = 0;
	  for(; i < presentModeCount; ++i ) {
	  	if(supportedPresentMode[i] == mode) {
	  		break;
	  	}
	  }
	  if( i < presentModeCount ) {
		  presentMode = mode;
		  break;
	  }
  }
	{
		VkSwapchainCreateInfoKHR swapChainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		swapChainCreateInfo.flags = 0;
		swapChainCreateInfo.surface = swapchain->vk.surface;
		swapChainCreateInfo.minImageCount = swapchain->imageCount;
		swapChainCreateInfo.imageFormat = selectedSurf->format;
		swapChainCreateInfo.imageColorSpace = selectedSurf->colorSpace;
		swapChainCreateInfo.imageExtent.width = init->width ;
		swapChainCreateInfo.imageExtent.height = init->height;
		swapChainCreateInfo.imageArrayLayers = 1;
		swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = NULL;
		swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainCreateInfo.presentMode = presentMode;
		swapChainCreateInfo.clipped = VK_TRUE;
		swapChainCreateInfo.oldSwapchain = 0;
		result = vkCreateSwapchainKHR( dev->vk.device, &swapChainCreateInfo, NULL, &swapchain->vk.swapchain);
	}

	{
		uint32_t imageNum = 0;
		vkGetSwapchainImagesKHR(dev->vk.device, swapchain->vk.swapchain, &imageNum, NULL);
		assert(imageNum <= RI_MAX_SWAPCHAIN_IMAGES);
		vkGetSwapchainImagesKHR(dev->vk.device, swapchain->vk.swapchain, &imageNum, swapchain->vk.images);
		for(size_t i = 0; i < imageNum; i++) {
			swapchain->textures[i].vk.image = swapchain->vk.images[i];
		}
		swapchain->imageCount = imageNum;
		swapchain->format = VKToRIFormat(selectedSurf->format);

		for(size_t i = 0; i < RI_MAX_SWAPCHAIN_IMAGES; i++) {
			VkSemaphoreCreateInfo createInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
      VkSemaphoreTypeCreateInfo timelineCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
      timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_BINARY;
			R_VK_ADD_STRUCT(&createInfo, &timelineCreateInfo);

			result = vkCreateSemaphore(dev->vk.device, &createInfo, NULL, &swapchain->vk.imageAcquireSem[i]);
			VK_WrapResult(result);
			
			result = vkCreateSemaphore(dev->vk.device, &createInfo, NULL, &swapchain->vk.finishSem[i]);
			VK_WrapResult(result);
		}
	}
	free(supportedPresentMode);
	free(surfaceFormats);
  return RI_SUCCESS;
}

uint32_t RISwapchainAcquireNextTexture( struct RIDevice_s *dev, struct RISwapchain_s *swapchain )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		VkSemaphore imageAcquiredSemaphore = swapchain->vk.imageAcquireSem[swapchain->vk.frameIndex];
		VK_WrapResult( vkAcquireNextImageKHR( dev->vk.device, swapchain->vk.swapchain, 5000 * 1000000ull, imageAcquiredSemaphore, VK_NULL_HANDLE, &swapchain->vk.textureIndex ) );
		return swapchain->vk.textureIndex;
	}
#endif
	return 0;
}

void RISwapchainPresent(struct RIDevice_s* dev, struct RISwapchain_s* swapchain) {
#if ( DEVICE_IMPL_VULKAN )
	{
		VkSemaphore imageAcquiredSemaphore = swapchain->vk.imageAcquireSem[swapchain->vk.frameIndex];
		VkSemaphore renderingFinishedSemaphore = swapchain->vk.finishSem[swapchain->vk.frameIndex];
		{ // Wait & Signal
			VkSemaphoreSubmitInfo waitSemaphore = { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
			waitSemaphore.semaphore = imageAcquiredSemaphore;
			waitSemaphore.stageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			VkSemaphoreSubmitInfo signalSemaphore = { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
			signalSemaphore.semaphore = renderingFinishedSemaphore;
			signalSemaphore.stageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			VkSubmitInfo2 submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
			submitInfo.waitSemaphoreInfoCount = 1;
			submitInfo.pWaitSemaphoreInfos = &waitSemaphore;
			submitInfo.signalSemaphoreInfoCount = 1;
			submitInfo.pSignalSemaphoreInfos = &signalSemaphore;
			VK_WrapResult( vkQueueSubmit2( swapchain->presentQueue->vk.queue, 1, &submitInfo, VK_NULL_HANDLE ) );
		}
		{
			VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &swapchain->vk.swapchain;
			presentInfo.pImageIndices = &swapchain->vk.textureIndex;

			VkPresentIdKHR presentId = { VK_STRUCTURE_TYPE_PRESENT_ID_KHR };
			presentId.swapchainCount = 1;
			presentId.pPresentIds = &swapchain->vk.presentID;

			if( dev->physicalAdapter.vk.isPresentIDSupported )
				presentInfo.pNext = &presentId;
			VK_WrapResult( vkQueuePresentKHR( swapchain->presentQueue->vk.queue, &presentInfo ) );
		}
		swapchain->vk.presentID++;
		swapchain->vk.frameIndex = ( swapchain->vk.frameIndex + 1 ) % RI_MAX_SWAPCHAIN_IMAGES;
	}
#endif
}

void FreeRISwapchain( struct RIDevice_s *dev, struct RISwapchain_s *swapchain )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		for( size_t i = 0; i < swapchain->imageCount; i++ ) {
			if( swapchain->vk.swapchain )
				vkDestroySwapchainKHR( dev->vk.device, swapchain->vk.swapchain, NULL );
			for( size_t i = 0; i < RI_MAX_SWAPCHAIN_IMAGES; i++ ) {
				if( swapchain->vk.imageAcquireSem[i] )
					vkDestroySemaphore( dev->vk.device, swapchain->vk.imageAcquireSem[i], NULL );
				if( swapchain->vk.finishSem[i] )
					vkDestroySemaphore( dev->vk.device, swapchain->vk.finishSem[i], NULL );
			}
			if( swapchain->vk.surface )
				vkDestroySurfaceKHR( dev->renderer->vk.instance, swapchain->vk.surface, NULL );
		}
	}
	memset( swapchain, 0, sizeof( struct RISwapchain_s ) );
#endif
}
