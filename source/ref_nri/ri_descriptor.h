#ifndef RI_DESCRIPTOR_H
#define RI_DESCRIPTOR_H

// Descriptor value types + the non-owning value builders. RIDescriptor_s is built by the
// RIDescriptor*() builders (defined in ri_renderer.c); RISampler_s / RIAccelStructure_s are the
// backend-owning resources it references. The builders are declared here (not in ri_renderer.h) so
// that every consumer of ri_types.h sees the prototype — a builder returns a >16-byte struct (sret
// ABI), so an implicit declaration at a call site silently corrupts the argument registers.

#include "ri_prelude.h"
#include "ri_resource.h" // RIBuffer_s / RITextureView_s (builder params)

struct RIDevice_s; // pointer-only param

// Backend-neutral descriptor type (RIDescriptor_s.type). Mapped to VkDescriptorType at bind via
// RI_VK_BindlessDescriptorType (ri_vk.h). Separate sampled images + samplers (no combined-image-sampler).
enum RIDescriptorType_e {
	RI_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
	RI_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	RI_DESCRIPTOR_TYPE_SAMPLER,
	RI_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	RI_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	RI_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE,
};

// A flags-free, non-owning value: the builders (RIDescriptor*() in ri_renderer.h) resolve the
// backend handle inline and derive `cookie` from the referenced resource's own cookie folded with
// the binding params. cookie == 0 means empty (RI_IsEmptyDescriptor). Ownership lives on the
// resource types (RIBuffer/RITexture/RITextureView/RISampler), never on the descriptor.
struct RIDescriptor_s {
	hash_t cookie;   // derived set-cache key; 0 == empty
	uint8_t type;    // RIDescriptorType_e
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			union {
				struct VkDescriptorImageInfo image;   // sampler + imageView + imageLayout
				struct VkDescriptorBufferInfo buffer; // buffer + offset + range
				VkAccelerationStructureKHR accelStructure;
			};
		} vk;
#endif
	};
};

// Owned backend sampler object; the only descriptor-referenced resource that owns a backend handle.
// Created/cached once (r_image.c sampler cache) and referenced non-owningly by descriptors.
struct RISampler_s {
	hash_t cookie;
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkSampler sampler;
		} vk;
#endif
	};
};

// Acceleration structure — plumbing only for now (no create path in warfork yet); present so the
// AS descriptor builder + bind-path write case match the reference design for forward compatibility.
struct RIAccelStructure_s {
	hash_t cookie;
	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkAccelerationStructureKHR handle;
		} vk;
#endif
	};
};

// Non-owning value builders. Each resolves the backend handle inline and derives the set-cache cookie
// from the referenced resource's own cookie folded with the binding params. A resource with cookie 0
// (uncreated) yields an empty descriptor (RI_IsEmptyDescriptor). `dev` is unused today (no resolution
// beyond a handle read) but kept for call-site stability.
struct RIDescriptor_s RIDescriptorUniformBuffer( struct RIDevice_s *dev, struct RIBuffer_s *buffer, uint64_t offset, uint64_t range );
struct RIDescriptor_s RIDescriptorStorageBuffer( struct RIDevice_s *dev, struct RIBuffer_s *buffer, uint64_t offset, uint64_t range );
// state: RIResourceState_e bits; selects the descriptor's image layout via RI_VK_ResourceStateToImageLayout.
struct RIDescriptor_s RIDescriptorSampledImage( struct RIDevice_s *dev, struct RITextureView_s *view, uint32_t state );
struct RIDescriptor_s RIDescriptorStorageImage( struct RIDevice_s *dev, struct RITextureView_s *view );
struct RIDescriptor_s RIDescriptorSampler( struct RIDevice_s *dev, struct RISampler_s *sampler );
struct RIDescriptor_s RIDescriptorAccelerationStructure( struct RIDevice_s *dev, struct RIAccelStructure_s *as );
static inline bool RI_IsEmptyDescriptor( struct RIDescriptor_s *desc )
{
	return desc->cookie == 0;
}

// RISampler ownership
void FreeRISampler( struct RIDevice_s *dev, struct RISampler_s *sampler );

#endif
