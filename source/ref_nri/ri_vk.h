#ifndef RI_VK_H
#define RI_VK_H

#include "ri_types.h"
#include "ri_format.h"

#if DEVICE_IMPL_VULKAN
#define RI_VK_DESCRIPTOR_IS_IMAGE( desc ) ( ( desc ).type == RI_DESCRIPTOR_TYPE_SAMPLER || ( desc ).type == RI_DESCRIPTOR_TYPE_STORAGE_IMAGE || ( desc ).type == RI_DESCRIPTOR_TYPE_SAMPLED_IMAGE )

// Backend-neutral RIDescriptorType_e -> VkDescriptorType, applied only at descriptor-set write time.
static inline VkDescriptorType RI_VK_BindlessDescriptorType( uint8_t type )
{
	switch( (enum RIDescriptorType_e)type ) {
		case RI_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case RI_DESCRIPTOR_TYPE_STORAGE_IMAGE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case RI_DESCRIPTOR_TYPE_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
		case RI_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case RI_DESCRIPTOR_TYPE_STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case RI_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE: return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	}
	return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

// state is a set of RIResourceState_e bits. GENERAL wins over the optimal layouts: a state that mixes
// storage access with anything else (e.g. a sampled view of a storage image) can only be satisfied by
// the GENERAL layout, so the order of these tests is load-bearing.
static inline VkImageLayout RI_VK_ResourceStateToImageLayout( uint32_t state )
{
	if( state & ( RI_RESOURCE_STATE_GENERAL | RI_RESOURCE_STATE_STORAGE_READ | RI_RESOURCE_STATE_STORAGE_WRITE | RI_RESOURCE_STATE_CLEAR_STORAGE ) )
		return VK_IMAGE_LAYOUT_GENERAL;
	if( state & ( RI_RESOURCE_STATE_RENDER_TARGET | RI_RESOURCE_STATE_RENDER_TARGET_READ ) )
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	if( state & RI_RESOURCE_STATE_DEPTH_WRITE )
		return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	if( state & RI_RESOURCE_STATE_DEPTH_READ )
		return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
	if( state & RI_RESOURCE_STATE_SHADER_RESOURCE )
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	if( state & RI_RESOURCE_STATE_COPY_SRC )
		return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	if( state & RI_RESOURCE_STATE_COPY_DST )
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	if( state & RI_RESOURCE_STATE_PRESENT )
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// UNDEFINED, or buffer-only / accel states that carry no image layout.
	return VK_IMAGE_LAYOUT_UNDEFINED;
}

static inline VkAccessFlags2 RI_VK_ResourceStateToAccess( uint32_t state )
{
	VkAccessFlags2 access = VK_ACCESS_2_NONE;
	if( state & RI_RESOURCE_STATE_GENERAL )
		access |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
	if( state & RI_RESOURCE_STATE_RENDER_TARGET )
		access |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	if( state & RI_RESOURCE_STATE_RENDER_TARGET_READ )
		access |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	if( state & RI_RESOURCE_STATE_DEPTH_WRITE )
		access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	if( state & RI_RESOURCE_STATE_DEPTH_READ )
		access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	if( state & RI_RESOURCE_STATE_SHADER_RESOURCE )
		access |= VK_ACCESS_2_SHADER_SAMPLED_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT;
	if( state & RI_RESOURCE_STATE_STORAGE_READ )
		access |= VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
	if( state & RI_RESOURCE_STATE_STORAGE_WRITE )
		access |= VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
	if( state & RI_RESOURCE_STATE_COPY_SRC )
		access |= VK_ACCESS_2_TRANSFER_READ_BIT;
	if( state & RI_RESOURCE_STATE_COPY_DST )
		access |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
	if( state & RI_RESOURCE_STATE_INDIRECT_ARGUMENT )
		access |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
	if( state & RI_RESOURCE_STATE_VERTEX_BUFFER )
		access |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
	if( state & RI_RESOURCE_STATE_INDEX_BUFFER )
		access |= VK_ACCESS_2_INDEX_READ_BIT;
	if( state & RI_RESOURCE_STATE_CONSTANT_BUFFER )
		access |= VK_ACCESS_2_UNIFORM_READ_BIT;
	if( state & RI_RESOURCE_STATE_ACCEL_READ )
		access |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	if( state & RI_RESOURCE_STATE_ACCEL_WRITE )
		access |= VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
	if( state & RI_RESOURCE_STATE_CLEAR_STORAGE )
		access |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
	if( state & RI_RESOURCE_STATE_HOST_READ )
		access |= VK_ACCESS_2_HOST_READ_BIT;
	return access;
}

// Conservative stage derivation for barriers that omit a stage hint.
static inline VkPipelineStageFlags2 RI_VK_ResourceStateToStages( uint32_t state )
{
	VkPipelineStageFlags2 flags = VK_PIPELINE_STAGE_2_NONE;
	if( state & ( RI_RESOURCE_STATE_GENERAL | RI_RESOURCE_STATE_SHADER_RESOURCE | RI_RESOURCE_STATE_STORAGE_READ |
	              RI_RESOURCE_STATE_STORAGE_WRITE | RI_RESOURCE_STATE_CONSTANT_BUFFER ) )
		flags |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
		         VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
	if( state & ( RI_RESOURCE_STATE_RENDER_TARGET | RI_RESOURCE_STATE_RENDER_TARGET_READ ) )
		flags |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	if( state & ( RI_RESOURCE_STATE_DEPTH_WRITE | RI_RESOURCE_STATE_DEPTH_READ ) )
		flags |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	if( state & ( RI_RESOURCE_STATE_COPY_SRC | RI_RESOURCE_STATE_COPY_DST ) )
		flags |= VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_CLEAR_BIT;
	if( state & RI_RESOURCE_STATE_INDIRECT_ARGUMENT )
		flags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
	if( state & ( RI_RESOURCE_STATE_VERTEX_BUFFER | RI_RESOURCE_STATE_INDEX_BUFFER ) )
		flags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
	if( state & ( RI_RESOURCE_STATE_ACCEL_READ | RI_RESOURCE_STATE_ACCEL_WRITE ) )
		flags |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
	if( state & RI_RESOURCE_STATE_CLEAR_STORAGE )
		flags |= VK_PIPELINE_STAGE_2_CLEAR_BIT;
	if( state & RI_RESOURCE_STATE_HOST_READ )
		flags |= VK_PIPELINE_STAGE_2_HOST_BIT;
	// UNDEFINED / PRESENT contribute no stages; that is what RI_BARRIER_STAGE_COLOR_ATTACHMENT exists
	// to work around on the first barrier after a swapchain acquire.
	return flags;
}

// hint is a set of RIBarrierStages_e bits; RI_BARRIER_STAGE_NONE derives from stateFallback instead.
static inline VkPipelineStageFlags2 RI_VK_BarrierStages( uint32_t hint, uint32_t stateFallback )
{
	if( hint == RI_BARRIER_STAGE_NONE )
		return RI_VK_ResourceStateToStages( stateFallback );

	VkPipelineStageFlags2 flags = VK_PIPELINE_STAGE_2_NONE;
	if( hint & RI_BARRIER_STAGE_VERTEX )
		flags |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
	if( hint & RI_BARRIER_STAGE_FRAGMENT )
		flags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	if( hint & RI_BARRIER_STAGE_COMPUTE )
		flags |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	if( hint & RI_BARRIER_STAGE_RAY_TRACING )
		flags |= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
	if( hint & RI_BARRIER_STAGE_DRAW_INDIRECT )
		flags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
	if( hint & RI_BARRIER_STAGE_COPY )
		flags |= VK_PIPELINE_STAGE_2_COPY_BIT;
	if( hint & RI_BARRIER_STAGE_BLIT )
		flags |= VK_PIPELINE_STAGE_2_BLIT_BIT;
	if( hint & RI_BARRIER_STAGE_CLEAR )
		flags |= VK_PIPELINE_STAGE_2_CLEAR_BIT;
	if( hint & RI_BARRIER_STAGE_ACCEL_BUILD )
		flags |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
	if( hint & RI_BARRIER_STAGE_COLOR_ATTACHMENT )
		flags |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	if( hint & RI_BARRIER_STAGE_HOST )
		flags |= VK_PIPELINE_STAGE_2_HOST_BIT;
	return flags;
}

static inline VkImageAspectFlags RI_VK_BarrierAspect( uint8_t aspect )
{
	switch( (enum RIBarrierAspect_e)aspect ) {
		case RI_BARRIER_ASPECT_COLOR: return VK_IMAGE_ASPECT_COLOR_BIT;
		case RI_BARRIER_ASPECT_DEPTH: return VK_IMAGE_ASPECT_DEPTH_BIT;
		case RI_BARRIER_ASPECT_STENCIL: return VK_IMAGE_ASPECT_STENCIL_BIT;
		case RI_BARRIER_ASPECT_DEPTH_STENCIL: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	return VK_IMAGE_ASPECT_COLOR_BIT;
}

static inline void RI_VK_FillColorAttachment( VkRenderingAttachmentInfo *info, struct RITextureView_s view, bool attachAndClear )
{
	info->imageView = view.vk.image;
	info->imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	info->resolveMode = VK_RESOLVE_MODE_NONE;
	info->resolveImageView = VK_NULL_HANDLE;
	info->resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info->loadOp = attachAndClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	info->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
}

static inline void RI_VK_FillDepthAttachment( VkRenderingAttachmentInfo *info, struct RITextureView_s view, bool attachAndClear )
{
	info->imageView = view.vk.image;
	info->imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	info->resolveMode = VK_RESOLVE_MODE_NONE;
	info->resolveImageView = VK_NULL_HANDLE;
	info->resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info->loadOp = attachAndClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	info->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	info->clearValue.depthStencil.depth = 1.0f;
}

const VkFormat RIFormatToVK(uint32_t format);
const enum RI_Format_e VKToRIFormat(VkFormat);  

#endif

#endif
