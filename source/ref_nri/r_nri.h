#ifndef R_NRI_IMP_H
#define R_NRI_IMP_H

#include "r_hasher.h"
#define NRI_STATIC_LIBRARY 1
#include "NRI.h"

#include "r_texture_format.h"
#include "r_vattribs.h"

#include "Extensions/NRIDeviceCreation.h"
#include "Extensions/NRIHelper.h"
#include "Extensions/NRIMeshShader.h"
#include "Extensions/NRIRayTracing.h"
#include "Extensions/NRISwapChain.h"
#include "Extensions/NRIWrapperVK.h"

#include "vulkan/vulkan.h"

const static NriSwapChainFormat DefaultSwapchainFormat = NriSwapChainFormat_BT709_G22_8BIT;
const static NriSPIRVBindingOffsets DefaultBindingOffset = { 100, 200, 300, 400 }; // just ShaderMake defaults for simplicity



// DirectX 12 requires ubo's be aligned by 256
const static uint32_t UBOBlockerBufferSize = 256 * 128;
const static uint32_t UBOBlockerBufferAlignmentReq = 256;

#define NUMBER_FRAMES_FLIGHT 3
#define NUMBER_RESERVED_BACKBUFFERS 4
#define DESCRIPTOR_MAX_BINDINGS 32
#define MAX_COLOR_ATTACHMENTS 8 
#define MAX_VERTEX_BINDINGS 24
#define MAX_PIPELINE_ATTACHMENTS 5
#define MAX_STREAMS 8 
#define MAX_ATTRIBUTES 32

#define BINDING_SETS_PER_POOL 24

enum descriptor_set_e { 
	DESCRIPTOR_SET_0, 
	DESCRIPTOR_SET_1, 
	DESCRIPTOR_SET_2, 
	DESCRIPTOR_SET_3, 
	DESCRIPTOR_SET_MAX 
};

#define R_VK_ABORT_ON_FAILURE( result )             \
	if( result != VK_SUCCESS ) {                    \
		Com_Printf( "invalid result: %d", result ); \
		exit( 0 );                                  \
	}

#define NRI_ABORT_ON_FAILURE( result )  \
	if( result != NriResult_SUCCESS ) { \
		exit( 0 );                      \
	}

#define COMMAND_BUFFER_COUNT 2

static const char *NriResultToString[NriResult_MAX_NUM] = { [NriResult_SUCCESS] = "SUCESS",
															[NriResult_FAILURE] = "FAILURE",
															[NriResult_INVALID_ARGUMENT] = "INVALID_ARGUMENT",
															[NriResult_OUT_OF_MEMORY] = "OUT_OF_MEMORY",
															[NriResult_UNSUPPORTED] = "UNSUPPORTED",
															[NriResult_DEVICE_LOST] = "DEVICE_LOST",
															[NriResult_OUT_OF_DATE] = "OUT_OF_DATE" };

static const char *NriDescriptorTypeToString[NriDescriptorType_MAX_NUM] = { [NriDescriptorType_SAMPLER] = "SAMPLER",
																			[NriDescriptorType_CONSTANT_BUFFER] = "CONSTANT_BUFFER",
																			[NriDescriptorType_TEXTURE] = "TEXTURE",
																			[NriDescriptorType_STORAGE_TEXTURE] = "STORAGE_TEXTURE",
																			[NriDescriptorType_BUFFER] = "BUFFER",
																			[NriDescriptorType_STORAGE_BUFFER] = "STORAGE_BUFFER",
																			[NriDescriptorType_STRUCTURED_BUFFER] = "STRUCTURED_BUFFER",
																			[NriDescriptorType_STORAGE_STRUCTURED_BUFFER] = "STORAGE_STRUCTURED_BUFFER",
																			[NriDescriptorType_ACCELERATION_STRUCTURE] = "ACCELERATION_STRUCTURE" };

// a wrapper to hold onto the hash + cookie
struct nri_descriptor_s {
	NriDescriptor *descriptor;
	uint32_t cookie;
};

struct nri_backend_s {
	NriGraphicsAPI api;
	NriHelperInterface helperI;
	NriCoreInterface coreI;
	NriSwapChainInterface swapChainI;
	NriWrapperVKInterface wrapperVKI;
	NriDevice *device;

	NriCommandQueue *graphicsCommandQueue;
	uint32_t cookie;
};

static inline struct nri_descriptor_s R_CreateDescriptorWrapper( struct nri_backend_s *backend, NriDescriptor *descriptor )
{
	return ( struct nri_descriptor_s ){ 
		.cookie = backend->cookie++, 
		.descriptor = descriptor 
	};
}

typedef struct {
	NriGraphicsAPI api;
	bool enableApiValidation;
	bool enableNriValidation;
} nri_init_desc_t;

bool R_InitNriBackend( const nri_init_desc_t *init, struct nri_backend_s *backend );
void R_NRI_CallbackMessage( NriMessage msg, const char *file, uint32_t line, const char *message, void *userArg );
NriFormat R_NRIFormat( enum texture_format_e format );


struct pipeline_layout_config_s {
	size_t numStreams;
	NriVertexStreamDesc streams[MAX_STREAMS];
	size_t numAttribs;
	NriVertexAttributeDesc attribs[MAX_ATTRIBUTES];

	//vattribmask_t attrib;
	//vattribmask_t halfAttrib;
	NriCullMode cullMode;

	size_t numFrameAttachments;
	NriViewport viewport;
	NriColorAttachmentDesc attachment[MAX_PIPELINE_ATTACHMENTS];
	NriDepthAttachmentDesc depthAttachment;
	NriStencilAttachmentDesc stencilAttachment;

	NriInputAssemblyDesc inputAssembly;
	NriRasterizationDesc rasterization;
};

#endif
