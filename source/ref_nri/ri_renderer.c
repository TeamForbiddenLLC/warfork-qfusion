#include "ri_renderer.h"

#include "stb_ds.h"
#include <qstr.h>

#include "ri_conversion.h"
#include "ri_gpu_preset.h"

#include "ri_types.h"

#if ( DEVICE_IMPL_VULKAN )

const static char *DefaultDeviceExtension[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_MAINTENANCE1_EXTENSION_NAME,
	VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
	VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME,
	VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME,
	VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
	VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,

	VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
	VK_EXT_DEVICE_FAULT_EXTENSION_NAME,
	// Fragment shader interlock extension to be used for ROV type functionality in Vulkan
	VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME,

	/************************************************************************/
	// AMD Specific Extensions
	/************************************************************************/
	VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
	VK_AMD_SHADER_BALLOT_EXTENSION_NAME,
	VK_AMD_GCN_SHADER_EXTENSION_NAME,
	VK_AMD_BUFFER_MARKER_EXTENSION_NAME,
	VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME,
	/************************************************************************/
	// Multi GPU Extensions
	/************************************************************************/
	VK_KHR_DEVICE_GROUP_EXTENSION_NAME,
	/************************************************************************/
	// Bindless & Non Uniform access Extensions
	/************************************************************************/
	VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
	VK_KHR_MAINTENANCE3_EXTENSION_NAME,
	// Required by raytracing and the new bindless descriptor API if we use it in future
	VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
	/************************************************************************/
	// Shader Atomic Int 64 Extension
	/************************************************************************/
	VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME,
	/************************************************************************/
	// Raytracing
	/************************************************************************/
	VK_KHR_RAY_QUERY_EXTENSION_NAME,
	VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
	// Required by VK_KHR_ray_tracing_pipeline
	VK_KHR_SPIRV_1_4_EXTENSION_NAME,
	// Required by VK_KHR_spirv_1_4
	VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,

	VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
	// Required by VK_KHR_acceleration_structure
	VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
	/************************************************************************/
	// YCbCr format support
	/************************************************************************/
	// Requirement for VK_KHR_sampler_ycbcr_conversion
	VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
	VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
	VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
	VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
	VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
	VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME,
	/************************************************************************/
	// Dynamic rendering
	/************************************************************************/
	VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
	VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME, // Required by VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
	VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,	 // Required by VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME
	VK_KHR_MULTIVIEW_EXTENSION_NAME,			 // Required by VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME
												 /************************************************************************/
	// Nsight Aftermath
	/************************************************************************/
	VK_EXT_ASTC_DECODE_MODE_EXTENSION_NAME,
};

void VK_ConfigureBufferQueueFamilies( VkBufferCreateInfo *info, struct RIQueue_s *queues, size_t numQueues, uint32_t *queueFamilies, size_t reservedLen )
{
	uint32_t uniqueQueue = 0;
	size_t queueFamilyIndexCount = 0;
	for( size_t i = 0; i < numQueues; i++ ) {
		if( queues[i].vk.queue ) {
			const uint32_t queueBit = ( 1 << queues[i].vk.queueFamilyIdx );
			if( ( uniqueQueue & queueBit ) > 0 ) {
				queueFamilies[queueFamilyIndexCount++] = queues[i].vk.queueFamilyIdx; // dev->queues[i].vk.queueFamilyIdx;
			}
			uniqueQueue |= queueBit;
		}
	}
	info->queueFamilyIndexCount = queueFamilyIndexCount;
	info->pQueueFamilyIndices = queueFamilies;
	info->sharingMode = ( queueFamilyIndexCount > 1 ) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
}

void VK_ConfigureImageQueueFamilies( VkImageCreateInfo *info, struct RIQueue_s *queues, size_t numQueues, uint32_t *queueFamilies, size_t reservedLen )
{
	uint32_t uniqueQueue = 0;
	size_t queueFamilyIndexCount = 0;
	for( size_t i = 0; i < numQueues; i++ ) {
		if( queues[i].vk.queue ) {
			const uint32_t queueBit = ( 1 << queues[i].vk.queueFamilyIdx );
			if( ( uniqueQueue & queueBit ) > 0 ) {
				queueFamilies[queueFamilyIndexCount++] = queues[i].vk.queueFamilyIdx; // dev->queues[i].vk.queueFamilyIdx;
			}
			uniqueQueue |= queueBit;
		}
	}
	info->queueFamilyIndexCount = queueFamilyIndexCount;
	info->pQueueFamilyIndices = queueFamilies;
	info->sharingMode = ( queueFamilyIndexCount > 1 ) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
}

void vk_fillQueueFamilies( struct RIDevice_s *dev, uint32_t *queueFamilies, uint32_t *queueFamiliesIdx, size_t reservedLen )
{
	uint32_t uniqueQueue = 0;
	for( size_t i = 0; i < RI_QUEUE_LEN; i++ ) {
		if( dev->queues[i].vk.queue ) {
			const uint32_t queueBit = ( 1 << dev->queues[i].vk.queueFamilyIdx );
			if( ( uniqueQueue & queueBit ) > 0 ) {
				assert( ( *queueFamiliesIdx ) < reservedLen );
				queueFamilies[( *queueFamiliesIdx )++] = dev->queues[i].vk.queueFamilyIdx;
			}
			uniqueQueue |= queueBit;
		}
	}
}

VkBool32 VKAPI_PTR __VK_DebugUtilsMessenger( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
											 VkDebugUtilsMessageTypeFlagsEXT messageType,
											 const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
											 void *userData )
{
	switch( messageSeverity ) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			//assert(callbackData->messageIdNumber ==0xc1c74a9c );
			Com_Printf( "VK ERROR: %s", callbackData->pMessage );
		  if( callbackData->messageIdNumber != 0xcc9c32be )
		  	assert( false );
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			Com_Printf( "VK WARNING: %s", callbackData->pMessage );
			break;
		default:
			Com_Printf( "VK INFO: %s", callbackData->pMessage );
			break;
	}
	return VK_FALSE;
}

inline static bool __VK_isExtensionNamesSupported( struct QStrSpan extension, const char **extensions, size_t len )
{
	for( size_t i = 0; i < len; i++ ) {
		if( qStrCompare( qCToStrRef( extensions[i] ), extension ) == 0 ) {
			return true;
		}
	}
	return false;
}

inline static bool __VK_isExtensionSupported( const char *targetExt, VkExtensionProperties *properties, size_t numExtensions )
{
	for( size_t i = 0; i < numExtensions; i++ ) {
		if( strcmp( properties[i].extensionName, targetExt ) == 0 ) {
			return true;
		}
	}
	return false;
}

static bool __VK_SupportExtension( VkExtensionProperties *properties, size_t len, struct QStrSpan extension )
{
	for( size_t i = 0; i < len; i++ ) {
		if( qStrCompare( qCToStrRef( (properties + i)->extensionName ), extension ) == 0 ) {
			return true;
		}
	}
	return false;
}

#endif

int EnumerateRIAdapters( struct RIRenderer_s *renderer, struct RIPhysicalAdapter_s *adapters, uint32_t *numAdapters )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		uint32_t deviceGroupNum = 0;
		if( !VK_WrapResult( vkEnumeratePhysicalDeviceGroups( renderer->vk.instance, &deviceGroupNum, NULL ) ) ) {
			return RI_FAIL;
		}

		if( adapters ) {
			VkPhysicalDeviceGroupProperties *physicalDeviceGroupProperties = calloc( deviceGroupNum, sizeof( VkPhysicalDeviceGroupProperties ) );
			for(size_t i = 0; i < deviceGroupNum; i++) {
				physicalDeviceGroupProperties[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES;
			}
			if( !VK_WrapResult(vkEnumeratePhysicalDeviceGroups( renderer->vk.instance, &deviceGroupNum, physicalDeviceGroupProperties ))) {
				free(physicalDeviceGroupProperties);
				return RI_FAIL;
			}
			assert( ( *numAdapters ) >= deviceGroupNum );
			for( size_t i = 0; i < deviceGroupNum; i++ ) {
				struct RIPhysicalAdapter_s *physicalAdapter = &adapters[i];
				memset( physicalAdapter, 0, sizeof( struct RIPhysicalAdapter_s ) );
				physicalAdapter->vk.physicalDevice = physicalDeviceGroupProperties[i].physicalDevices[0];

				uint32_t extensionNum = 0;
				vkEnumerateDeviceExtensionProperties( physicalAdapter->vk.physicalDevice, NULL, &extensionNum, NULL );
				VkExtensionProperties *extensionProperties = malloc( extensionNum * sizeof( VkExtensionProperties ) );
				vkEnumerateDeviceExtensionProperties( physicalAdapter->vk.physicalDevice, NULL, &extensionNum, extensionProperties );

				VkPhysicalDeviceProperties2 properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
				VkPhysicalDeviceVulkan11Properties props11 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES };
				VkPhysicalDeviceVulkan12Properties props12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES };
				VkPhysicalDeviceVulkan13Properties props13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES };
				VkPhysicalDeviceIDProperties deviceIDProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES };
				R_VK_ADD_STRUCT( &properties, &props11 );
				R_VK_ADD_STRUCT( &properties, &props12 );
				R_VK_ADD_STRUCT( &properties, &props13 );
				R_VK_ADD_STRUCT( &properties, &deviceIDProperties );

				VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
				VkPhysicalDeviceVulkan11Features features11 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
				VkPhysicalDeviceVulkan12Features features12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
				VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };

				R_VK_ADD_STRUCT( &features, &features11 );
				R_VK_ADD_STRUCT( &features, &features12 );

				VkPhysicalDevicePresentIdFeaturesKHR presentIdFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR };
				if( __VK_SupportExtension( extensionProperties, extensionNum, qCToStrRef( VK_KHR_PRESENT_ID_EXTENSION_NAME ) ) ) {
					R_VK_ADD_STRUCT( &features, &features13 );
				}

				VkPhysicalDeviceMemoryProperties memoryProperties = { 0 };
				vkGetPhysicalDeviceMemoryProperties( physicalAdapter->vk.physicalDevice, &memoryProperties );
				vkGetPhysicalDeviceProperties2( physicalAdapter->vk.physicalDevice, &properties );
				vkGetPhysicalDeviceFeatures2( physicalAdapter->vk.physicalDevice, &features );

				// Fill desc
				physicalAdapter->luid = *(uint64_t *)&deviceIDProperties.deviceLUID[0];
				physicalAdapter->deviceId = properties.properties.deviceID;
				physicalAdapter->vendor = VendorFromID( properties.properties.vendorID );
				physicalAdapter->vk.physicalDevice = physicalAdapter->vk.physicalDevice;
				physicalAdapter->vk.apiVersion = properties.properties.apiVersion;
				physicalAdapter->presetLevel = RI_GPU_PRESET_NONE;
				// selected preset
				for(size_t i = 0; i < Q_ARRAY_COUNT(gpuPCPresets); i++) {
					if(gpuPCPresets[i].vendorId == properties.properties.vendorID && 
						 gpuPCPresets[i].modelId == properties.properties.deviceID) {
						physicalAdapter->presetLevel = gpuPCPresets[i].preset;
						break;
					}
				}

				switch( properties.properties.deviceType ) {
					case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
						physicalAdapter->type = RI_ADAPTER_TYPE_INTEGRATED_GPU;
						break;
					case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
						physicalAdapter->type = RI_ADAPTER_TYPE_DISCRETE_GPU;
						break;
					case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
						physicalAdapter->type = RI_ADAPTER_TYPE_VIRTUAL_GPU;
						break;
					case VK_PHYSICAL_DEVICE_TYPE_CPU:
						physicalAdapter->type = RI_ADAPTER_TYPE_CPU;
						break;
					case VK_PHYSICAL_DEVICE_TYPE_OTHER:
					default:
						physicalAdapter->type = RI_ADAPTER_TYPE_OTHER;
						break;
				}

				physicalAdapter->vk.isSwapChainSupported = __VK_SupportExtension( extensionProperties, extensionNum, qCToStrRef( VK_KHR_SWAPCHAIN_EXTENSION_NAME ) );

				physicalAdapter->vk.isPresentIDSupported = presentIdFeatures.presentId;
				physicalAdapter->vk.isBufferDeviceAddressSupported =
					physicalAdapter->vk.apiVersion >= VK_API_VERSION_1_2 || __VK_SupportExtension( extensionProperties, extensionNum, qCToStrRef( VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME ) );
				physicalAdapter->vk.isBufferDeviceAddressSupported = __VK_SupportExtension( extensionProperties, extensionNum, qCToStrRef( VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME ) );

				const VkPhysicalDeviceLimits *limits = &properties.properties.limits;

				physicalAdapter->viewportMaxNum = limits->maxViewports;
				physicalAdapter->viewportBoundsRange[0] = limits->viewportBoundsRange[0];
				physicalAdapter->viewportBoundsRange[1] = limits->viewportBoundsRange[1];

				physicalAdapter->attachmentMaxDim = Q_MIN( limits->maxFramebufferWidth, limits->maxFramebufferHeight );
				physicalAdapter->attachmentLayerMaxNum = limits->maxFramebufferLayers;
				physicalAdapter->colorAttachmentMaxNum = limits->maxColorAttachments;

				physicalAdapter->colorSampleMaxNum = limits->framebufferColorSampleCounts;
				physicalAdapter->depthSampleMaxNum = limits->framebufferDepthSampleCounts;
				physicalAdapter->stencilSampleMaxNum = limits->framebufferStencilSampleCounts;
				physicalAdapter->zeroAttachmentsSampleMaxNum = limits->framebufferNoAttachmentsSampleCounts;
				physicalAdapter->textureColorSampleMaxNum = limits->sampledImageColorSampleCounts;
				physicalAdapter->textureIntegerSampleMaxNum = limits->sampledImageIntegerSampleCounts;
				physicalAdapter->textureDepthSampleMaxNum = limits->sampledImageDepthSampleCounts;
				physicalAdapter->textureStencilSampleMaxNum = limits->sampledImageStencilSampleCounts;
				physicalAdapter->storageTextureSampleMaxNum = limits->storageImageSampleCounts;

				physicalAdapter->texture1DMaxDim = limits->maxImageDimension1D;
				physicalAdapter->texture2DMaxDim = limits->maxImageDimension2D;
				physicalAdapter->texture3DMaxDim = limits->maxImageDimension3D;
				physicalAdapter->textureArrayLayerMaxNum = limits->maxImageArrayLayers;
				physicalAdapter->typedBufferMaxDim = limits->maxTexelBufferElements;

				for( uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++ ) {
					// const VkMemoryType& memoryType = m_MemoryProps.memoryTypes[i];
					if( ( memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ) != 0 && physicalAdapter->type != RI_ADAPTER_TYPE_INTEGRATED_GPU )
						physicalAdapter->videoMemorySize += memoryProperties.memoryHeaps[i].size;
					else
						physicalAdapter->systemMemorySize += memoryProperties.memoryHeaps[i].size;
					const uint32_t uploadHeapFlags = ( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
					if( ( memoryProperties.memoryHeaps[i].flags & uploadHeapFlags ) == uploadHeapFlags )
						physicalAdapter->deviceUploadHeapSize += memoryProperties.memoryHeaps[i].size;
				}

				physicalAdapter->memoryAllocationMaxNum = limits->maxMemoryAllocationCount;
				physicalAdapter->samplerAllocationMaxNum = limits->maxSamplerAllocationCount;
				physicalAdapter->constantBufferMaxRange = limits->maxUniformBufferRange;
				physicalAdapter->storageBufferMaxRange = limits->maxStorageBufferRange;
				physicalAdapter->bufferTextureGranularity = (uint32_t)limits->bufferImageGranularity;
				physicalAdapter->bufferMaxSize = props13.maxBufferSize;

				physicalAdapter->uploadBufferTextureRowAlignment = (uint32_t)limits->optimalBufferCopyRowPitchAlignment;
				physicalAdapter->uploadBufferTextureSliceAlignment = (uint32_t)limits->optimalBufferCopyOffsetAlignment; // TODO: ?
				physicalAdapter->bufferShaderResourceOffsetAlignment = (uint32_t)Q_MAX( limits->minTexelBufferOffsetAlignment, limits->minStorageBufferOffsetAlignment );
				physicalAdapter->constantBufferOffsetAlignment = (uint32_t)limits->minUniformBufferOffsetAlignment;
				// physicalAdapter->scratchBufferOffsetAlignment = accelerationStructureProps.minAccelerationStructureScratchOffsetAlignment;
				// physicalAdapter->shaderBindingTableAlignment = rayTracingProps.shaderGroupBaseAlignment;

				physicalAdapter->pipelineLayoutDescriptorSetMaxNum = limits->maxBoundDescriptorSets;
				physicalAdapter->pipelineLayoutRootConstantMaxSize = limits->maxPushConstantsSize;
				// physicalAdapter->pipelineLayoutRootDescriptorMaxNum = pushDescriptorProps.maxPushDescriptors;

				physicalAdapter->perStageDescriptorSamplerMaxNum = limits->maxPerStageDescriptorSamplers;
				physicalAdapter->perStageDescriptorConstantBufferMaxNum = limits->maxPerStageDescriptorUniformBuffers;
				physicalAdapter->perStageDescriptorStorageBufferMaxNum = limits->maxPerStageDescriptorStorageBuffers;
				physicalAdapter->perStageDescriptorTextureMaxNum = limits->maxPerStageDescriptorSampledImages;
				physicalAdapter->perStageDescriptorStorageTextureMaxNum = limits->maxPerStageDescriptorStorageImages;
				physicalAdapter->perStageResourceMaxNum = limits->maxPerStageResources;

				physicalAdapter->descriptorSetSamplerMaxNum = limits->maxDescriptorSetSamplers;
				physicalAdapter->descriptorSetConstantBufferMaxNum = limits->maxDescriptorSetUniformBuffers;
				physicalAdapter->descriptorSetStorageBufferMaxNum = limits->maxDescriptorSetStorageBuffers;
				physicalAdapter->descriptorSetTextureMaxNum = limits->maxDescriptorSetSampledImages;
				physicalAdapter->descriptorSetStorageTextureMaxNum = limits->maxDescriptorSetStorageImages;

				physicalAdapter->vertexShaderAttributeMaxNum = limits->maxVertexInputAttributes;
				physicalAdapter->vertexShaderStreamMaxNum = limits->maxVertexInputBindings;
				physicalAdapter->vertexShaderOutputComponentMaxNum = limits->maxVertexOutputComponents;

				physicalAdapter->tessControlShaderGenerationMaxLevel = (float)limits->maxTessellationGenerationLevel;
				physicalAdapter->tessControlShaderPatchPointMaxNum = limits->maxTessellationPatchSize;
				physicalAdapter->tessControlShaderPerVertexInputComponentMaxNum = limits->maxTessellationControlPerVertexInputComponents;
				physicalAdapter->tessControlShaderPerVertexOutputComponentMaxNum = limits->maxTessellationControlPerVertexOutputComponents;
				physicalAdapter->tessControlShaderPerPatchOutputComponentMaxNum = limits->maxTessellationControlPerPatchOutputComponents;
				physicalAdapter->tessControlShaderTotalOutputComponentMaxNum = limits->maxTessellationControlTotalOutputComponents;
				physicalAdapter->tessEvaluationShaderInputComponentMaxNum = limits->maxTessellationEvaluationInputComponents;
				physicalAdapter->tessEvaluationShaderOutputComponentMaxNum = limits->maxTessellationEvaluationOutputComponents;

				physicalAdapter->geometryShaderInvocationMaxNum = limits->maxGeometryShaderInvocations;
				physicalAdapter->geometryShaderInputComponentMaxNum = limits->maxGeometryInputComponents;
				physicalAdapter->geometryShaderOutputComponentMaxNum = limits->maxGeometryOutputComponents;
				physicalAdapter->geometryShaderOutputVertexMaxNum = limits->maxGeometryOutputVertices;
				physicalAdapter->geometryShaderTotalOutputComponentMaxNum = limits->maxGeometryTotalOutputComponents;

				physicalAdapter->fragmentShaderInputComponentMaxNum = limits->maxFragmentInputComponents;
				physicalAdapter->fragmentShaderOutputAttachmentMaxNum = limits->maxFragmentOutputAttachments;
				physicalAdapter->fragmentShaderDualSourceAttachmentMaxNum = limits->maxFragmentDualSrcAttachments;

				physicalAdapter->computeShaderSharedMemoryMaxSize = limits->maxComputeSharedMemorySize;
				physicalAdapter->computeShaderWorkGroupMaxNum[0] = limits->maxComputeWorkGroupCount[0];
				physicalAdapter->computeShaderWorkGroupMaxNum[1] = limits->maxComputeWorkGroupCount[1];
				physicalAdapter->computeShaderWorkGroupMaxNum[2] = limits->maxComputeWorkGroupCount[2];
				physicalAdapter->computeShaderWorkGroupInvocationMaxNum = limits->maxComputeWorkGroupInvocations;
				physicalAdapter->computeShaderWorkGroupMaxDim[0] = limits->maxComputeWorkGroupSize[0];
				physicalAdapter->computeShaderWorkGroupMaxDim[1] = limits->maxComputeWorkGroupSize[1];
				physicalAdapter->computeShaderWorkGroupMaxDim[2] = limits->maxComputeWorkGroupSize[2];

				// physicalAdapter->rayTracingShaderGroupIdentifierSize = rayTracingProps.shaderGroupHandleSize;
				// physicalAdapter->rayTracingShaderTableMaxStride = rayTracingProps.maxShaderGroupStride;
				// physicalAdapter->rayTracingShaderRecursionMaxDepth = rayTracingProps.maxRayRecursionDepth;
				// physicalAdapter->rayTracingGeometryObjectMaxNum = (uint32_t)accelerationStructureProps.maxGeometryCount;

				// physicalAdapter->meshControlSharedMemoryMaxSize = meshShaderProps.maxTaskSharedMemorySize;
				// physicalAdapter->meshControlWorkGroupInvocationMaxNum = meshShaderProps.maxTaskWorkGroupInvocations;
				// physicalAdapter->meshControlPayloadMaxSize = meshShaderProps.maxTaskPayloadSize;
				// physicalAdapter->meshEvaluationOutputVerticesMaxNum = meshShaderProps.maxMeshOutputVertices;
				// physicalAdapter->meshEvaluationOutputPrimitiveMaxNum = meshShaderProps.maxMeshOutputPrimitives;
				// physicalAdapter->meshEvaluationOutputComponentMaxNum = meshShaderProps.maxMeshOutputComponents;
				// physicalAdapter->meshEvaluationSharedMemoryMaxSize = meshShaderProps.maxMeshSharedMemorySize;
				// physicalAdapter->meshEvaluationWorkGroupInvocationMaxNum = meshShaderProps.maxMeshWorkGroupInvocations;

				physicalAdapter->viewportPrecisionBits = limits->viewportSubPixelBits;
				physicalAdapter->subPixelPrecisionBits = limits->subPixelPrecisionBits;
				physicalAdapter->subTexelPrecisionBits = limits->subTexelPrecisionBits;
				physicalAdapter->mipmapPrecisionBits = limits->mipmapPrecisionBits;

				physicalAdapter->timestampFrequencyHz = (uint64_t)( 1e9 / (double)limits->timestampPeriod + 0.5 );
				physicalAdapter->drawIndirectMaxNum = limits->maxDrawIndirectCount;
				physicalAdapter->samplerLodBiasMin = -limits->maxSamplerLodBias;
				physicalAdapter->samplerLodBiasMax = limits->maxSamplerLodBias;
				physicalAdapter->samplerAnisotropyMax = limits->maxSamplerAnisotropy;
				physicalAdapter->texelOffsetMin = limits->minTexelOffset;
				physicalAdapter->texelOffsetMax = limits->maxTexelOffset;
				physicalAdapter->texelGatherOffsetMin = limits->minTexelGatherOffset;
				physicalAdapter->texelGatherOffsetMax = limits->maxTexelGatherOffset;
				physicalAdapter->clipDistanceMaxNum = limits->maxClipDistances;
				physicalAdapter->cullDistanceMaxNum = limits->maxCullDistances;
				physicalAdapter->combinedClipAndCullDistanceMaxNum = limits->maxCombinedClipAndCullDistances;
				// physicalAdapter->shadingRateAttachmentTileSize = (uint8_t)shadingRateProps.minFragmentShadingRateAttachmentTexelSize.width;

				// Based on https://docs.vulkan.org/guide/latest/hlsl.html#_shader_model_coverage // TODO: code below needs to be improved
				// physicalAdapter->shaderModel = 51;
				// if (physicalAdapter->isShaderNativeI64Supported)
				//    physicalAdapter->shaderModel = 60;
				// if (features11.multiview)
				//    physicalAdapter->shaderModel = 61;
				// if (physicalAdapter->isShaderNativeF16Supported || physicalAdapter->isShaderNativeI16Supported)
				//    physicalAdapter->shaderModel = 62;
				// if (physicalAdapter->isRayTracingSupported)
				//    physicalAdapter->shaderModel = 63;
				// if (physicalAdapter->shadingRateTier >= 2)
				//    physicalAdapter->shaderModel = 64;
				// if (physicalAdapter->isMeshShaderSupported || physicalAdapter->rayTracingTier >= 2)
				//    physicalAdapter->shaderModel = 65;
				// if (physicalAdapter->isShaderAtomicsI64Supported)
				//    physicalAdapter->shaderModel = 66;
				// if (features.features.shaderStorageImageMultisample)
				//    physicalAdapter->shaderModel = 67;

				// if (physicalAdapter->conservativeRasterTier) {
				//     if (conservativeRasterProps.primitiveOverestimationSize < 1.0f / 2.0f && conservativeRasterProps.degenerateTrianglesRasterized)
				//         physicalAdapter->conservativeRasterTier = 2;
				//     if (conservativeRasterProps.primitiveOverestimationSize <= 1.0 / 256.0f && conservativeRasterProps.degenerateTrianglesRasterized)
				//         physicalAdapter->conservativeRasterTier = 3;
				// }

				// if (physicalAdapter->sampleLocationsTier) {
				//     if (sampleLocationsProps.variableSampleLocations) // TODO: it's weird...
				//         physicalAdapter->sampleLocationsTier = 2;
				// }

				// if (physicalAdapter->rayTracingTier) {
				//     if (rayTracingPipelineFeatures.rayTracingPipelineTraceRaysIndirect && rayQueryFeatures.rayQuery)
				//         physicalAdapter->rayTracingTier = 2;
				// }

				// if (physicalAdapter->shadingRateTier) {
				//     physicalAdapter->isAdditionalShadingRatesSupported = shadingRateProps.maxFragmentSize.height > 2 || shadingRateProps.maxFragmentSize.width > 2;
				//     if (shadingRateFeatures.primitiveFragmentShadingRate && shadingRateFeatures.attachmentFragmentShadingRate)
				//         physicalAdapter->shadingRateTier = 2;
				// }

				physicalAdapter->bindlessTier = features12.descriptorIndexing ? 1 : 0;

				physicalAdapter->isTextureFilterMinMaxSupported = features12.samplerFilterMinmax;
				physicalAdapter->isLogicFuncSupported = features.features.logicOp;
				physicalAdapter->isDepthBoundsTestSupported = features.features.depthBounds;
				physicalAdapter->isDrawIndirectCountSupported = features12.drawIndirectCount;
				physicalAdapter->isIndependentFrontAndBackStencilReferenceAndMasksSupported = true;
				// physicalAdapter->isLineSmoothingSupported = lineRasterizationFeatures.smoothLines;
				physicalAdapter->isCopyQueueTimestampSupported = limits->timestampComputeAndGraphics;
				// physicalAdapter->isMeshShaderPipelineStatsSupported = meshShaderFeatures.meshShaderQueries == VK_TRUE;
				physicalAdapter->isEnchancedBarrierSupported = true;
				physicalAdapter->isMemoryTier2Supported = true; // TODO: seems to be the best match
				physicalAdapter->isDynamicDepthBiasSupported = true;
				physicalAdapter->isViewportOriginBottomLeftSupported = true;
				physicalAdapter->isRegionResolveSupported = true;

				physicalAdapter->isShaderNativeI16Supported = features.features.shaderInt16;
				physicalAdapter->isShaderNativeF16Supported = features12.shaderFloat16;
				physicalAdapter->isShaderNativeI32Supported = true;
				physicalAdapter->isShaderNativeF32Supported = true;
				physicalAdapter->isShaderNativeI64Supported = features.features.shaderInt64;
				physicalAdapter->isShaderNativeF64Supported = features.features.shaderFloat64;
				// physicalAdapter->isShaderAtomicsF16Supported = (shaderAtomicFloat2Features.shaderBufferFloat16Atomics || shaderAtomicFloat2Features.shaderSharedFloat16Atomics) ? true : false;
				physicalAdapter->isShaderAtomicsI32Supported = true;
				// physicalAdapter->isShaderAtomicsF32Supported = (shaderAtomicFloatFeatures.shaderBufferFloat32Atomics || shaderAtomicFloatFeatures.shaderSharedFloat32Atomics) ? true : false;
				physicalAdapter->isShaderAtomicsI64Supported = ( features12.shaderBufferInt64Atomics || features12.shaderSharedInt64Atomics ) ? true : false;
				// physicalAdapter->isShaderAtomicsF64Supported = (shaderAtomicFloatFeatures.shaderBufferFloat64Atomics || shaderAtomicFloatFeatures.shaderSharedFloat64Atomics) ? true : false;

				free( extensionProperties );
			}
			free( physicalDeviceGroupProperties );
		} else {
			( *numAdapters ) = deviceGroupNum;
		}
	}
#endif
	return RI_SUCCESS;
}

static inline VkDeviceQueueCreateInfo* __VK_findQueueCreateInfo(VkDeviceQueueCreateInfo* queues, size_t numQueues, uint32_t queueIndex) {
	for( size_t i = 0; i < numQueues; i++ ) {
		if( queues[i].queueFamilyIndex == queueIndex ) {
			return queues + i;
		}
	}
	return NULL;
}

int InitRIDevice( struct RIRenderer_s *renderer, struct RIDeviceDesc_s *init, struct RIDevice_s *device )
{
	assert(device);
	assert( init->physicalAdapter );
	memset(device, 0, sizeof(struct RIDevice_s));

	enum RIResult_e riResult = RI_SUCCESS;
	struct RIPhysicalAdapter_s *physicalAdapter = init->physicalAdapter;
	
	device->renderer = renderer;
	device->physicalAdapter = *init->physicalAdapter;

#if ( DEVICE_IMPL_VULKAN )
	{
		const char **enabledExtensionNames = NULL;

		uint32_t extensionNum = 0;
		vkEnumerateDeviceExtensionProperties( physicalAdapter->vk.physicalDevice, NULL, &extensionNum, NULL );
		VkExtensionProperties *extensionProperties = malloc( extensionNum * sizeof( VkExtensionProperties ) );
		vkEnumerateDeviceExtensionProperties( physicalAdapter->vk.physicalDevice, NULL, &extensionNum, extensionProperties );
		
		for(size_t i = 0; i < extensionNum; i++) {
			Com_Printf( "VK Extension %s - %lu", extensionProperties[i].extensionName, extensionProperties[i].specVersion);
		}

		uint32_t familyNum = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( init->physicalAdapter->vk.physicalDevice, &familyNum, NULL );

		VkQueueFamilyProperties *queueFamilyProps = malloc( ( familyNum * sizeof( VkQueueFamilyProperties ) ) );
		vkGetPhysicalDeviceQueueFamilyProperties( init->physicalAdapter->vk.physicalDevice, &familyNum, queueFamilyProps );
		
		VkDeviceQueueCreateInfo deviceQueueCreateInfo[8] = { 0 };
		VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfo;
		const float priorities[] = {1.0f, 0.9f, 0.8f, 0.7f, 0.6f, 0.5f};

		{
			struct QStr str = { 0 };
			uint8_t numFeatures = 0;
			struct QStrSpan queueFeatures[9];
			for( size_t i = 0; i < familyNum; i++ ) {
				qStrSetLen( &str, 0 );
				numFeatures = 0;
				if(queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) 
					queueFeatures[numFeatures++] = qCToStrRef("VK_QUEUE_GRAPHICS_BIT"); 
				if(queueFamilyProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT ) 
					queueFeatures[numFeatures++] = qCToStrRef("VK_QUEUE_COMPUTE_BIT"); 
				if(queueFamilyProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT) 
					queueFeatures[numFeatures++] = qCToStrRef("VK_QUEUE_TRANSFER_BIT"); 
				if(queueFamilyProps[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) 
					queueFeatures[numFeatures++] = qCToStrRef("VK_QUEUE_SPARSE_BINDING_BIT"); 
				if(queueFamilyProps[i].queueFlags & VK_QUEUE_PROTECTED_BIT) 
					queueFeatures[numFeatures++] = qCToStrRef("VK_QUEUE_PROTECTED_BIT"); 
				if(queueFamilyProps[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) 
					queueFeatures[numFeatures++] = qCToStrRef("VK_QUEUE_VIDEO_DECODE_BIT_KHR"); 
				if(queueFamilyProps[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR ) 
					queueFeatures[numFeatures++] = qCToStrRef("VK_QUEUE_VIDEO_ENCODE_BIT_KHR"); 
				if(queueFamilyProps[i].queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV ) 
					queueFeatures[numFeatures++] = qCToStrRef("VK_QUEUE_OPTICAL_FLOW_BIT_NV"); 
				qstrcatprintf(&str, "VK Queue - %lu: ", i);
				qstrcatjoin(&str, queueFeatures, numFeatures, qCToStrRef(","));
				Com_Printf("%.*s", str.len, str.buf);
			}
			qStrFree( &str );
		}

		struct {
			uint32_t requiredBits;
			uint8_t queueType;
		} configureQueue[] = {
			{ VK_QUEUE_GRAPHICS_BIT, RI_QUEUE_GRAPHICS },
			{ VK_QUEUE_COMPUTE_BIT, RI_QUEUE_COMPUTE },
			{ VK_QUEUE_TRANSFER_BIT, RI_QUEUE_COPY },
		};
		for( uint32_t configureIdx = 0; configureIdx < Q_ARRAY_COUNT( configureQueue ); configureIdx++ ) {
			//bool found = false;
			const uint32_t requiredBits = configureQueue[configureIdx].requiredBits;

			uint32_t minQueueFlag = UINT32_MAX;
			uint32_t bestQueueFamilyIdx = 0;
			for( size_t familyIdx = 0; familyIdx < familyNum; familyIdx++ ) {
				// for the graphics queue we select the first avaliable
				if( configureQueue[configureIdx].queueType == RI_QUEUE_GRAPHICS && ( configureQueue[configureIdx].requiredBits & queueFamilyProps[familyIdx].queueFlags ) > 0 ) {
					bestQueueFamilyIdx = familyIdx;
					break;
				}
				VkDeviceQueueCreateInfo* createInfo = __VK_findQueueCreateInfo(deviceQueueCreateInfo, deviceCreateInfo.queueCreateInfoCount, familyIdx);
				if( queueFamilyProps[familyIdx].queueCount == 0 ) {
					continue;
				}

				const uint32_t matchingQueueFlags = ( queueFamilyProps[familyIdx].queueFlags & requiredBits );
				// Example: Required flag is VK_QUEUE_TRANSFER_BIT and the queue family has only VK_QUEUE_TRANSFER_BIT set
				if( matchingQueueFlags && ( ( queueFamilyProps[familyIdx].queueFlags & ~requiredBits ) == 0 ) &&
					( queueFamilyProps[familyIdx].queueCount - ( createInfo ? createInfo->queueCount : 0 ) ) > 0 ) {
					bestQueueFamilyIdx = familyIdx;
					break;
				}

				// Queue family 1 has VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT
				// Queue family 2 has VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_SPARSE_BINDING_BIT
				// Since 1 has less flags, we choose queue family 1
				if( matchingQueueFlags && ( ( queueFamilyProps[familyIdx].queueFlags - matchingQueueFlags ) < minQueueFlag ) ) {
					bestQueueFamilyIdx = familyIdx;
					minQueueFlag = ( queueFamilyProps[familyIdx].queueFlags - matchingQueueFlags );
				}
			}

			VkDeviceQueueCreateInfo *createInfo = __VK_findQueueCreateInfo( deviceQueueCreateInfo, deviceCreateInfo.queueCreateInfoCount, bestQueueFamilyIdx);
			if(createInfo == NULL)
				createInfo = &deviceQueueCreateInfo[deviceCreateInfo.queueCreateInfoCount++];
			createInfo->queueFamilyIndex = bestQueueFamilyIdx;
			createInfo->pQueuePriorities = priorities;
			createInfo->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

			struct RIQueue_s *queue = &device->queues[configureQueue[configureIdx].queueType];
			if( createInfo->queueCount >= queueFamilyProps[createInfo->queueFamilyIndex].queueCount) {
				struct RIQueue_s *dupQueue = NULL;
				minQueueFlag = UINT32_MAX;
				for( size_t i = 0; i < Q_ARRAY_COUNT( device->queues ); i++ ) {
					const uint32_t matchingQueueFlags = ( device->queues[i].vk.queueFlags & requiredBits  );
					if( matchingQueueFlags && ( ( device->queues[i].vk.queueFlags & ~requiredBits  ) == 0 ) ) {
						dupQueue = &device->queues[i];
						break;
					}

					if( matchingQueueFlags && ( ( device->queues[i].vk.queueFlags - matchingQueueFlags ) < minQueueFlag ) ) {
						minQueueFlag = ( device->queues[i].vk.queueFlags - matchingQueueFlags );
						dupQueue = &device->queues[i];
					}
				}
				if( dupQueue ) {
					device->queues[configureQueue[configureIdx].queueType] = *dupQueue;
				}
			} else {
				queue->vk.queueFlags = queueFamilyProps[createInfo->queueFamilyIndex].queueFlags;
				queue->vk.slotIdx = createInfo->queueCount++;
				queue->vk.queueFamilyIdx = createInfo->queueFamilyIndex;
			}
		}

		//for( uint32_t initIdx = 0; initIdx < Q_ARRAY_COUNT( configureQueue ); initIdx++ ) {
		//	VkDeviceQueueCreateInfo *selectedQueue = NULL;
		//	bool found = false;
		//	uint32_t minQueueFlag = UINT32_MAX;
		//	const uint32_t requiredFlags = configureQueue[initIdx].requiredBits;
		//	for( size_t familyIdx = 0; familyIdx < familyNum; familyIdx++ ) {
		//		uint32_t avaliableQueues = 0;
		//		size_t createQueueIdx = 0;
		//		for( ; createQueueIdx < Q_ARRAY_COUNT( deviceQueueCreateInfo ); createQueueIdx++ ) {
		//			const bool foundQueueFamily = deviceQueueCreateInfo[createQueueIdx].queueFamilyIndex == familyIdx;
		//			const bool isQueueEmpty =(deviceQueueCreateInfo[createQueueIdx].queueCount == 0); 
		//			if( foundQueueFamily || isQueueEmpty) {
		//				selectedQueue = &deviceQueueCreateInfo[createQueueIdx];
		//				if(isQueueEmpty) {
		//					deviceCreateInfo.queueCreateInfoCount = Q_MAX( deviceCreateInfo.queueCreateInfoCount, createQueueIdx + 1);
		//					selectedQueue->pQueuePriorities = priorities;
		//					selectedQueue->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		//				}
		//				selectedQueue->queueFamilyIndex = familyIdx;
		//				avaliableQueues = queueFamilyProps[familyIdx].queueCount - selectedQueue->queueCount;
		//				break;
		//			}
		//		}

		//		// for the graphics queue we select the first avaliable
		//		if( configureQueue[initIdx].queueType == RI_QUEUE_GRAPHICS && ( configureQueue[initIdx].requiredBits & queueFamilyProps[familyIdx].queueFlags ) > 0 ) {
		//			found = true;
		//			break;
		//		}

		//		assert( createQueueIdx < Q_ARRAY_COUNT( deviceQueueCreateInfo ) );
		//		if( avaliableQueues == 0 ) {
		//			continue; // skip queue family there is no more avaliable
		//		}
		//		const uint32_t matchingQueueFlags = ( queueFamilyProps[familyIdx].queueFlags & requiredFlags );

		//		// Example: Required flag is VK_QUEUE_TRANSFER_BIT and the queue family has only VK_QUEUE_TRANSFER_BIT set
		//		if( matchingQueueFlags && ( ( queueFamilyProps[familyIdx].queueFlags & ~requiredFlags ) == 0 ) && avaliableQueues > 0 ) {
		//			found = true;
		//			break;
		//		}

		//		// Queue family 1 has VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT
		//		// Queue family 2 has VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_SPARSE_BINDING_BIT
		//		// Since 1 has less flags, we choose queue family 1
		//		if( matchingQueueFlags && ( ( queueFamilyProps[familyIdx].queueFlags - matchingQueueFlags ) < minQueueFlag ) ) {
		//			found = true;
		//			minQueueFlag = ( queueFamilyProps[familyIdx].queueFlags - matchingQueueFlags );
		//		}
		//	}

		//	if( found ) {
		//		struct RIQueue_s *queue = &device->queues[configureQueue[initIdx].queueType];
		//		queue->vk.queueFlags = queueFamilyProps[selectedQueue->queueFamilyIndex].queueFlags;
		//		queue->vk.slotIdx = selectedQueue->queueCount++;
		//		queue->vk.queueFamilyIdx = selectedQueue->queueFamilyIndex;
		//	} else {
		//		struct RIQueue_s *dupQueue = NULL;
		//		minQueueFlag = UINT32_MAX;
		//		for( size_t i = 0; i < Q_ARRAY_COUNT( device->queues ); i++ ) {
		//			const uint32_t matchingQueueFlags = ( device->queues[i].vk.queueFlags & requiredFlags );
		//			if( matchingQueueFlags && ( ( device->queues[i].vk.queueFlags & ~requiredFlags ) == 0 ) ) {
		//				dupQueue = &device->queues[i];
		//				break;
		//			}

		//			if( matchingQueueFlags && ( ( device->queues[i].vk.queueFlags - matchingQueueFlags ) < minQueueFlag ) ) {
		//				found = true;
		//				minQueueFlag = ( device->queues[i].vk.queueFlags - matchingQueueFlags );
		//				dupQueue = &device->queues[i];
		//			}
		//		}
		//		if( dupQueue ) {
		//			device->queues[configureQueue[initIdx].queueType] = *dupQueue;
		//		}
		//	}
		//}

		VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

		VkPhysicalDeviceVulkan11Features features11 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
		R_VK_ADD_STRUCT( &features, &features11 );

		VkPhysicalDeviceVulkan12Features features12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		R_VK_ADD_STRUCT( &features, &features12 );

		VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		if( renderer->vk.apiVersion >= VK_API_VERSION_1_3 ) {
			R_VK_ADD_STRUCT( &features, &features13 );
		}

		VkPhysicalDeviceMaintenance5FeaturesKHR maintenance5Features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR };
		if( __VK_isExtensionNamesSupported( qCToStrRef( VK_KHR_MAINTENANCE_5_EXTENSION_NAME ), enabledExtensionNames, arrlen( enabledExtensionNames ) ) ) {
			R_VK_ADD_STRUCT( &features, &maintenance5Features );
			device->vk.maintenance5Features = true;
		}

		// VkPhysicalDeviceFragmentShadingRateFeaturesKHR shadingRateFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR };
		// if( __VK_isExtensionSupported( VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, , extensionProperties, extensionNum ) ) {
		//	APPEND_EXT( shadingRateFeatures );
		// }

		VkPhysicalDevicePresentIdFeaturesKHR presentIdFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR };
		if( __VK_isExtensionNamesSupported( qCToStrRef( VK_KHR_PRESENT_ID_EXTENSION_NAME ), enabledExtensionNames, arrlen( enabledExtensionNames ) ) ) {
			R_VK_ADD_STRUCT( &features, &presentIdFeatures );
		}

		VkPhysicalDevicePresentWaitFeaturesKHR presentWaitFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR };
		if( __VK_isExtensionNamesSupported( qCToStrRef( VK_KHR_PRESENT_WAIT_EXTENSION_NAME ), enabledExtensionNames, arrlen( enabledExtensionNames ) ) ) {
			R_VK_ADD_STRUCT( &features, &presentWaitFeatures );
		}

		VkPhysicalDeviceLineRasterizationFeaturesKHR lineRasterizationFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_KHR };
		if( __VK_isExtensionNamesSupported( qCToStrRef( VK_KHR_LINE_RASTERIZATION_EXTENSION_NAME ), enabledExtensionNames, arrlen( enabledExtensionNames ) ) ) {
			R_VK_ADD_STRUCT( &features, &lineRasterizationFeatures );
		}

		//VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
		//if( IsExtensionSupported( VK_EXT_MESH_SHADER_EXTENSION_NAME, desiredDeviceExts ) ) {
		//	APPEND_EXT( meshShaderFeatures );
		//}

	 // VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
   // if (IsExtensionSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, desiredDeviceExts)) {
   //     APPEND_EXT(accelerationStructureFeatures);
   // }

   // VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
   // if (IsExtensionSupported(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, desiredDeviceExts)) {
   //     APPEND_EXT(rayTracingPipelineFeatures);
   // }

   // VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
   // if (IsExtensionSupported(VK_KHR_RAY_QUERY_EXTENSION_NAME, desiredDeviceExts)) {
   //     APPEND_EXT(rayQueryFeatures);
   // }

   // VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR rayTracingMaintenanceFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR};
   // if (IsExtensionSupported(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME, desiredDeviceExts)) {
   //     APPEND_EXT(rayTracingMaintenanceFeatures);
   // }

   // VkPhysicalDeviceOpacityMicromapFeaturesEXT micromapFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_FEATURES_EXT};
   // if (IsExtensionSupported(VK_EXT_OPACITY_MICROMAP_EXTENSION_NAME, desiredDeviceExts)) {
   //     APPEND_EXT(micromapFeatures);
   // }

   // VkPhysicalDeviceShaderAtomicFloatFeaturesEXT shaderAtomicFloatFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT};
   // if (IsExtensionSupported(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME, desiredDeviceExts)) {
   //     APPEND_EXT(shaderAtomicFloatFeatures);
   // }

   // VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT shaderAtomicFloat2Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT};
   // if (IsExtensionSupported(VK_EXT_SHADER_ATOMIC_FLOAT_2_EXTENSION_NAME, desiredDeviceExts)) {
   //     APPEND_EXT(shaderAtomicFloat2Features);
   // }

   // VkPhysicalDeviceMemoryPriorityFeaturesEXT memoryPriorityFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT};
   // if (IsExtensionSupported(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME, desiredDeviceExts)) {
   //     APPEND_EXT(memoryPriorityFeatures);
   // }

   // VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT slicedViewFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_SLICED_VIEW_OF_3D_FEATURES_EXT};
   // if (IsExtensionSupported(VK_EXT_IMAGE_SLICED_VIEW_OF_3D_EXTENSION_NAME, desiredDeviceExts)) {
   //     APPEND_EXT(slicedViewFeatures);
   // }

   // VkPhysicalDeviceCustomBorderColorFeaturesEXT borderColorFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT};
   // if (IsExtensionSupported(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME, desiredDeviceExts)) {
   //     APPEND_EXT(borderColorFeatures);
   // }

		vkGetPhysicalDeviceFeatures2( physicalAdapter->vk.physicalDevice, &features );
		for( size_t idx = 0; idx < Q_ARRAY_COUNT( DefaultDeviceExtension ); idx++ ) {
			if( __VK_SupportExtension( extensionProperties, extensionNum, qCToStrRef( DefaultDeviceExtension[idx] ) ) ) {
				Com_Printf("Enabled Extension: %s", extensionProperties[idx].extensionName);
				arrpush( enabledExtensionNames, DefaultDeviceExtension[idx] );
			}
		}

		deviceCreateInfo.pNext = &features;
		deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfo;
		deviceCreateInfo.enabledExtensionCount = (uint32_t)arrlen( enabledExtensionNames );
		deviceCreateInfo.ppEnabledExtensionNames = enabledExtensionNames;

		VkResult result = vkCreateDevice( physicalAdapter->vk.physicalDevice, &deviceCreateInfo, NULL, &device->vk.device );
		if( !VK_WrapResult( result ) ) {
			riResult = RI_FAIL;
			goto vk_done;
		}

		// the request size
		for( size_t q = 0; q < Q_ARRAY_COUNT( device->queues ); q++ ) {
			// the queue
			if( device->queues[q].vk.queueFlags == 0 )
				continue;
			vkGetDeviceQueue( device->vk.device, device->queues[q].vk.queueFamilyIdx, device->queues[q].vk.slotIdx, &device->queues[q].vk.queue );
		}

		{
			VmaVulkanFunctions vulkanFunctions = { 0 };
			vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
			vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
			vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
			vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
			vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
			vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
			vulkanFunctions.vkFreeMemory = vkFreeMemory;
			vulkanFunctions.vkMapMemory = vkMapMemory;
			vulkanFunctions.vkUnmapMemory = vkUnmapMemory;
			vulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
			vulkanFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
			vulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
			vulkanFunctions.vkBindImageMemory = vkBindImageMemory;
			vulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
			vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
			vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
			vulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
			vulkanFunctions.vkCreateImage = vkCreateImage;
			vulkanFunctions.vkDestroyImage = vkDestroyImage;
			vulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;
			/// Fetch "vkGetBufferMemoryRequirements2" on Vulkan >= 1.1, fetch "vkGetBufferMemoryRequirements2KHR" when using VK_KHR_dedicated_allocation extension.
			vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
			/// Fetch "vkGetImageMemoryRequirements2" on Vulkan >= 1.1, fetch "vkGetImageMemoryRequirements2KHR" when using VK_KHR_dedicated_allocation extension.
			vulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;
			/// Fetch "vkBindBufferMemory2" on Vulkan >= 1.1, fetch "vkBindBufferMemory2KHR" when using VK_KHR_bind_memory2 extension.
			vulkanFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR;
			/// Fetch "vkBindImageMemory2" on Vulkan >= 1.1, fetch "vkBindImageMemory2KHR" when using VK_KHR_bind_memory2 extension.
			vulkanFunctions.vkBindImageMemory2KHR = vkBindImageMemory2KHR;
			/// Fetch from "vkGetPhysicalDeviceMemoryProperties2" on Vulkan >= 1.1, but you can also fetch it from "vkGetPhysicalDeviceMemoryProperties2KHR" if you enabled extension
			/// VK_KHR_get_physical_device_properties2.
			vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
			/// Fetch from "vkGetDeviceBufferMemoryRequirements" on Vulkan >= 1.3, but you can also fetch it from "vkGetDeviceBufferMemoryRequirementsKHR" if you enabled extension VK_KHR_maintenance4.
			vulkanFunctions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
			/// Fetch from "vkGetDeviceImageMemoryRequirements" on Vulkan >= 1.3, but you can also fetch it from "vkGetDeviceImageMemoryRequirementsKHR" if you enabled extension VK_KHR_maintenance4.
			vulkanFunctions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;

			VmaAllocatorCreateInfo createInfo = { 0 };
			createInfo.physicalDevice = device->physicalAdapter.vk.physicalDevice;
			createInfo.device = device->vk.device;
			createInfo.instance = device->renderer->vk.instance;
			createInfo.pVulkanFunctions = &vulkanFunctions;
			createInfo.vulkanApiVersion = VK_API_VERSION_1_3 ;

			if( device->physicalAdapter.vk.isBufferDeviceAddressSupported ) {
				createInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
			}

			if( device->physicalAdapter.vk.isAMDDeviceCoherentMemorySupported ) {
				createInfo.flags |= VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT;
			}

			result = vmaCreateAllocator( &createInfo, &device->vk.vmaAllocator );
			if( !VK_WrapResult( result ) ) {
				riResult = RI_FAIL;
				goto vk_done;
			}
		}

	vk_done:
		free( queueFamilyProps );
		free( extensionProperties );
		arrfree( enabledExtensionNames );
	}
#endif
	return riResult;
}

int InitRIRenderer( const struct RIBackendInit_s *init, struct RIRenderer_s *renderer )
{
	memset(renderer, 0, sizeof(struct RIRenderer_s));
	renderer->api = init->api;
#if(DEVICE_IMPL_VULKAN)
	{
		volkInitialize();

		VkApplicationInfo appInfo = { 0 };
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = NULL;
		appInfo.pApplicationName = init->applicationName;
		appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
		appInfo.pEngineName = "qfusion";
		appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.apiVersion = VK_API_VERSION_1_3;

    renderer->vk.apiVersion = appInfo.apiVersion;

		const VkValidationFeatureEnableEXT enabledValidationFeatures[] = { VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };

		VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
		validationFeatures.enabledValidationFeatureCount = Q_ARRAY_COUNT( enabledValidationFeatures );
		validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures;

		VkInstanceCreateInfo instanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		instanceCreateInfo.pApplicationInfo = &appInfo;
		const char *enabledLayerNames[8] = { 0 };
		const char *enabledExtensionNames[8] = { 0 };
		instanceCreateInfo.ppEnabledLayerNames = enabledLayerNames;
		instanceCreateInfo.enabledLayerCount = 0;
		instanceCreateInfo.ppEnabledExtensionNames = enabledExtensionNames;
		instanceCreateInfo.enabledExtensionCount = 0;

		VkLayerProperties *layerProperties = NULL;
		VkExtensionProperties *extProperties = NULL;
		{
			assert( 1 <= Q_ARRAY_COUNT( enabledLayerNames ) );
			uint32_t enumInstanceLayers = 0;
			vkEnumerateInstanceLayerProperties( &enumInstanceLayers, NULL );
			layerProperties = malloc( enumInstanceLayers * sizeof( VkLayerProperties ) );
			vkEnumerateInstanceLayerProperties( &enumInstanceLayers, layerProperties );
			for( size_t i = 0; i < enumInstanceLayers; i++ ) {
				bool useLayer = false;
				useLayer |= ( init->vk.enableValidationLayer && strcmp( layerProperties[i].layerName, "VK_LAYER_KHRONOS_validation" ) == 0 );
				Com_Printf( "Instance Layer: %s(%d): %s", layerProperties[i].layerName, layerProperties[i].specVersion, useLayer ? "ENABLED" : "DISABLED" );
				if( useLayer ) {
					assert( instanceCreateInfo.enabledLayerCount < Q_ARRAY_COUNT( enabledLayerNames ) );
					enabledLayerNames[instanceCreateInfo.enabledLayerCount++] = layerProperties[i].layerName;
				}
			}
		}
		{
			uint32_t extensionNum = 0;
			vkEnumerateInstanceExtensionProperties( NULL, &extensionNum, NULL );
			extProperties = malloc( extensionNum * sizeof( VkExtensionProperties ) );
			vkEnumerateInstanceExtensionProperties( NULL, &extensionNum, extProperties );

			const bool supportSurfaceExtension = __VK_isExtensionSupported( VK_KHR_SURFACE_EXTENSION_NAME, extProperties, extensionNum );
			for( size_t i = 0; i < extensionNum; i++ ) {
				bool useExtension = false;

				if( supportSurfaceExtension ) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
					useExtension |= ( strcmp( extProperties[i].extensionName, VK_KHR_WIN32_SURFACE_EXTENSION_NAME ) == 0 );
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
					useExtension |= ( strcmp( extProperties[i].extensionName, VK_EXT_METAL_SURFACE_EXTENSION_NAME ) == 0 );
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
					useExtension |= ( strcmp( extProperties[i].extensionName, VK_KHR_XLIB_SURFACE_EXTENSION_NAME ) == 0 );
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
					useExtension |= ( strcmp( extProperties[i].extensionName, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME ) == 0 );
#endif
				}
				useExtension |= ( strcmp( extProperties[i].extensionName, VK_KHR_SURFACE_EXTENSION_NAME ) == 0 );
				useExtension |= ( strcmp( extProperties[i].extensionName, VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME ) == 0 );
				useExtension |= ( strcmp( extProperties[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) == 0 );
				Com_Printf( "Instance Extensions: %s(%d): %s", extProperties[i].extensionName, extProperties[i].specVersion, useExtension ? "ENABLED" : "DISABLED" );
				if( useExtension ) {
					assert( instanceCreateInfo.enabledExtensionCount < Q_ARRAY_COUNT( enabledExtensionNames ) );
					enabledExtensionNames[instanceCreateInfo.enabledExtensionCount++] = extProperties[i].extensionName;
				}
			}
		}

		if( init->vk.enableValidationLayer ) {
			R_VK_ADD_STRUCT( &instanceCreateInfo, &validationFeatures );
		}

		VkResult result = vkCreateInstance( &instanceCreateInfo, NULL, &renderer->vk.instance );
		free( layerProperties );
		free( extProperties );
		if( !VK_WrapResult( result ) ) {
			return RI_FAIL;
		}
		volkLoadInstance(renderer->vk.instance);
		if( init->vk.enableValidationLayer && vkCreateDebugUtilsMessengerEXT) {
			VkDebugUtilsMessengerCreateInfoEXT createInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
			createInfo.pUserData = renderer;
			createInfo.pfnUserCallback = __VK_DebugUtilsMessenger;

			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
			createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
			createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			vkCreateDebugUtilsMessengerEXT( renderer->vk.instance, &createInfo, NULL, &renderer->vk.debugMessageUtils );
		}
	}
#endif
	return RI_SUCCESS;
}

void UpdateRIDescriptor( struct RIDevice_s *dev, struct RIDescriptor_s *desc )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		switch( desc->vk.type ) {
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			case VK_DESCRIPTOR_TYPE_SAMPLER:
				// test some assumptions
				assert( desc->vk.type == VK_DESCRIPTOR_TYPE_SAMPLER ||
						( desc->texture && ( desc->vk.type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || desc->vk.type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ) ) );
				desc->cookie = hash_data( hash_u64( HASH_INITIAL_VALUE, desc->vk.type ), &desc->vk.image, sizeof( desc->vk.image ) );
				break;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				assert( desc->buffer );
				desc->vk.buffer.buffer = desc->buffer->vk.buffer;
				desc->cookie = hash_data( hash_u64( HASH_INITIAL_VALUE, desc->vk.type ), &desc->vk.buffer, sizeof( desc->vk.buffer ) );
				break;
			default:
				assert( false );
				break;
		}
	}
#endif
}

void FreeRIDescriptor( struct RIDevice_s *dev, struct RIDescriptor_s *desc )
{
#if ( DEVICE_IMPL_VULKAN )
	switch( desc->vk.type ) {
		case VK_DESCRIPTOR_TYPE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			if( desc->vk.image.sampler && ( desc->flags & RI_VK_DESC_OWN_SAMPLER ) )
				vkDestroySampler( dev->vk.device, desc->vk.image.sampler, NULL );
			if( desc->vk.image.imageView && ( desc->flags & RI_VK_DESC_OWN_IMAGE_VIEW ) )
				vkDestroyImageView( dev->vk.device, desc->vk.image.imageView, NULL );
			break;
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			break;
		default:
			break;
	}
#endif

	memset( desc, 0, sizeof( struct RIDescriptor_s ) );
}

void FreeRIFree(struct RIDevice_s* dev,struct RIFree_s* mem) {
assert(free);
	switch(mem->type) {
#if ( DEVICE_IMPL_VULKAN )
		case RI_FREE_VK_IMAGE:
			vkDestroyImage( dev->vk.device, mem->vkImage, NULL);
			break;
		case RI_FREE_VK_IMAGEVIEW:
			vkDestroyImageView(dev->vk.device, mem->vkImageView, NULL);
			break;
		case RI_FREE_VK_SAMPLER:
			vkDestroySampler(dev->vk.device, mem->vkSampler, NULL);
			break;
		case RI_FREE_VK_VMA_AllOC:
			vmaFreeMemory(dev->vk.vmaAllocator, mem->vmaAlloc);
			break;
		case RI_FREE_VK_BUFFER:
			vkDestroyBuffer(dev->vk.device, mem->vkBuffer, NULL);
			break;
		case RI_FREE_VK_BUFFER_VIEW:
			vkDestroyBufferView(dev->vk.device, mem->vkBufferView, NULL);
			break;
#endif
		default:
			assert( false ); // invalid type to free
			break;
	}
}

void FreeRITexture( struct RIDevice_s *dev, struct RITexture_s *tex )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		if( tex->vk.image ) {
			vkDestroyImage( dev->vk.device, tex->vk.image, NULL );
			tex->vk.image = NULL;
		}
	}
#endif
}

void FreeRICmd(struct RIDevice_s* dev, struct RICmd_s* cmd) {
	assert(cmd->vk.pool);
	assert(cmd->vk.cmd);
#if ( DEVICE_IMPL_VULKAN )
	{
		if( cmd->vk.cmd) {
			vkFreeCommandBuffers( dev->vk.device, cmd->vk.pool, 1, &cmd->vk.cmd );
		}
		cmd->vk.cmd = VK_NULL_HANDLE;
		cmd->vk.pool = VK_NULL_HANDLE;
	}
#endif

}


void ShutdownRIRenderer( struct RIRenderer_s *renderer )
{
#if ( DEVICE_IMPL_VULKAN )
	if( renderer->vk.debugMessageUtils )
		vkDestroyDebugUtilsMessengerEXT( renderer->vk.instance, renderer->vk.debugMessageUtils, NULL );
	vkDestroyInstance( renderer->vk.instance, NULL );
	volkFinalize();
#endif
}

void WaitRIQueueIdle( struct RIDevice_s *device, struct RIQueue_s *queue ) {
#if ( DEVICE_IMPL_VULKAN )
	VK_WrapResult( vkQueueWaitIdle( queue->vk.queue ) );
#endif
}

int FreeRIDevice( struct RIDevice_s *dev ) {
#if ( DEVICE_IMPL_VULKAN )
	vkDestroyDevice( dev->vk.device, NULL );
	if( dev->vk.vmaAllocator )
		vmaDestroyAllocator( dev->vk.vmaAllocator );
#endif
}
