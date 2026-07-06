#ifndef RI_PRELUDE_H
#define RI_PRELUDE_H

// Shared foundation for the ri_* component headers: base includes, the Vulkan/VMA loader, the VK
// result-wrapping helpers, queue-flag bits, and cross-cutting defines/enums. Every ri_*.h type
// header includes this; nothing here depends on another ri_* header.

#include "../gameshared/q_arch.h"
#include "math/qmath.h"
#include "qhash.h"
#include "qtypes.h"

#define RI_MAX_SWAPCHAIN_IMAGES 8
#define RI_NUMBER_FRAMES_FLIGHT 3

#include "ri_defines.h"

#ifdef DEVICE_SUPPORT_VULKAN
#include "volk.h"
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc.h"
#endif

#define R_VK_ADD_STRUCT( current, next )                \
	{                                                   \
		void *__pNext = (void *)( ( current )->pNext ); \
		( current )->pNext = ( next );                  \
		( next )->pNext = __pNext;                      \
	}
#define VK_WrapResult( res ) __vk_WrapResult( res, __FILE__, __FUNCTION__, __LINE__ )

static inline bool __vk_WrapResult( VkResult result, const char *sourceFilename, const char *functionName, int sourceLine )
{
	if( result != VK_SUCCESS ) {

		printf( "RI: VK %i, file %s:%i (%s)\n", result, sourceFilename, sourceLine, functionName );
		return false;
	}
	return true;
}

#define RI_QUEUE_GRAPHICS_BIT 0x1
#define RI_QUEUE_COMPUTE_BIT 0x2
#define RI_QUEUE_TRANSFER_BIT 0x4
#define RI_QUEUE_SPARSE_BINDING_BIT 0x8
#define RI_QUEUE_VIDEO_DECODE_BIT 0x10
#define RI_QUEUE_VIDEO_ENCODE_BIT 0x20
#define RI_QUEUE_PROTECTED_BIT 0x40
#define RI_QUEUE_OPTICAL_FLOW_BIT_NV 0x80
#define RI_QUEUE_INVALID 0x0

enum RIResult_e { RI_INCOMPLETE_DEVICE = -2, RI_FAIL = -1, RI_SUCCESS = 0, RI_INCOMPLETE };

#endif
