#include "ri_timeline.h"

int InitRITimeline( struct RIDevice_s *dev, struct RITimeline_s *timeline )
{
	memset( timeline, 0, sizeof( struct RITimeline_s ) );
#if ( DEVICE_IMPL_VULKAN )
	{
		VkSemaphoreTypeCreateInfo timelineCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
		timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		timelineCreateInfo.initialValue = 0;
		VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		R_VK_ADD_STRUCT( &createInfo, &timelineCreateInfo );
		if( !VK_WrapResult( vkCreateSemaphore( dev->vk.device, &createInfo, NULL, &timeline->vk.semaphore ) ) )
			return RI_FAIL;
	}
#endif
	return RI_SUCCESS;
}

void FreeRITimeline( struct RIDevice_s *dev, struct RITimeline_s *timeline )
{
#if ( DEVICE_IMPL_VULKAN )
	if( timeline->vk.semaphore )
		vkDestroySemaphore( dev->vk.device, timeline->vk.semaphore, NULL );
#endif
	memset( timeline, 0, sizeof( struct RITimeline_s ) );
}

uint64_t RITimelineNext( struct RITimeline_s *timeline )
{
	return ++timeline->signalValue;
}

uint64_t RITimelinePending( const struct RITimeline_s *timeline )
{
	return timeline->signalValue;
}

uint64_t RITimelineCompleted( struct RIDevice_s *dev, struct RITimeline_s *timeline )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		uint64_t value = 0;
		VK_WrapResult( vkGetSemaphoreCounterValue( dev->vk.device, timeline->vk.semaphore, &value ) );
		return value;
	}
#endif
	return timeline->signalValue;
}

void RITimelineWait( struct RIDevice_s *dev, struct RITimeline_s *timeline, uint64_t value )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		VkSemaphoreWaitInfo waitInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
		waitInfo.semaphoreCount = 1;
		waitInfo.pSemaphores = &timeline->vk.semaphore;
		waitInfo.pValues = &value;
		VK_WrapResult( vkWaitSemaphores( dev->vk.device, &waitInfo, UINT64_MAX ) );
	}
#endif
}
