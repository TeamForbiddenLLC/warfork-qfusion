/*
Copyright (C) 2013 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef R_PROGRAM_H
#define R_PROGRAM_H

#include "r_frame_cmd_buffer.h"
typedef uint64_t r_glslfeat_t;

#include "../gameshared/q_arch.h"
#include "r_math.h"
#include "r_vattribs.h"
#include "r_resource.h"

#include "r_descriptor_pool.h"
#include "ri_types.h"
#include "qhash.h"

#define GLSL_BIT(x)							(1ULL << (x))
#define GLSL_BITS_VERSION					17

#define PIPELINE_LAYOUT_HASH_SIZE 4096// need to handle this large number of pipelines 
#define PIPELINE_REFLECTION_HASH_SIZE 64
#define VERTEX_POS_BINDING_SLOT (0)

#define DEFAULT_GLSL_MATERIAL_PROGRAM			"defaultMaterial"
#define DEFAULT_GLSL_DISTORTION_PROGRAM			"defaultDistortion"
#define DEFAULT_GLSL_RGB_SHADOW_PROGRAM			"defaultRGBShadow"
#define DEFAULT_GLSL_SHADOWMAP_PROGRAM			"defaultShadowmap"
#define DEFAULT_GLSL_OUTLINE_PROGRAM			"defaultOutline"
#define DEFAULT_GLSL_DYNAMIC_LIGHTS_PROGRAM		"defaultDynamicLights"
#define DEFAULT_GLSL_Q3A_SHADER_PROGRAM			"defaultQ3AShader"
#define DEFAULT_GLSL_CELSHADE_PROGRAM			"defaultCelshade"
#define DEFAULT_GLSL_FOG_PROGRAM				"defaultFog"
#define DEFAULT_GLSL_FXAA_PROGRAM				"defaultFXAA"
#define DEFAULT_GLSL_YUV_PROGRAM				"defaultYUV"
#define DEFAULT_GLSL_COLORCORRECTION_PROGRAM	"defaultColorCorrection"

#if ( DEVICE_IMPL_MTL )
// Which MSL index space a descriptor lands in. Metal keeps buffers, textures and samplers in three
// separate per-stage argument tables, so a descriptor's slot number is only meaningful with its class.
enum r_mtl_bind_class_e {
	R_MTL_BIND_BUFFER,
	R_MTL_BIND_TEXTURE,
	R_MTL_BIND_SAMPLER,
	R_MTL_BIND_NONE = 0xFF,
};
#define R_MTL_SLOT_UNUSED 0xFF

// A slot the frontend does not fill still has to be written with *something* type-compatible: Metal
// rejects a texture2d encoded into a texturecube argument. Only the shapes the shaders actually declare
// are distinguished; anything else falls back to 2D and is caught by validation if it ever appears.
enum r_mtl_tex_dim_e {
	R_MTL_TEX_2D,
	R_MTL_TEX_CUBE,
};

// Vertex streams live at the top of the MSL buffer index space so they can never collide with the
// descriptor buffers assigned from 0 upwards. Metal guarantees 31 buffer bind points per stage, and
// MAX_STREAMS is 8, so 16..23 is comfortably inside the guarantee with room for descriptors below.
#define R_MTL_VERTEX_BUFFER_BASE 16

// Sets that become argument buffers are pinned to MSL buffer indices from 0 upward, densely, in set
// order. This has to be *low*: SPIRV-Cross emits the argument-buffer arguments first and then does
// next_metal_resource_index_buffer = max(next, ab_index + 1) before auto-assigning every remaining
// buffer (the discrete sets' UBOs and the push-constant block). Pinning high would push those up into
// the vertex range above and past Metal's 31-slot guarantee. SPIRV-Cross says the same in spirv_msl.hpp
// ("should be kept as small as possible").
#define R_MTL_ARG_BUFFER_BASE 0

#endif

typedef enum glsl_program_type_s
{
	GLSL_PROGRAM_TYPE_NONE,
	GLSL_PROGRAM_TYPE_MATERIAL,
	GLSL_PROGRAM_TYPE_DISTORTION,
	GLSL_PROGRAM_TYPE_RGB_SHADOW,
	GLSL_PROGRAM_TYPE_SHADOWMAP,
	GLSL_PROGRAM_TYPE_OUTLINE,
	GLSL_PROGRAM_TYPE_UNUSED,
	GLSL_PROGRAM_TYPE_Q3A_SHADER,
	GLSL_PROGRAM_TYPE_CELSHADE,
	GLSL_PROGRAM_TYPE_FOG,
	GLSL_PROGRAM_TYPE_FXAA,
	GLSL_PROGRAM_TYPE_YUV,
	GLSL_PROGRAM_TYPE_COLORCORRECTION,

	GLSL_PROGRAM_TYPE_MAXTYPE
} glsl_program_type_t;

// features common for all program types
#define GLSL_SHADER_COMMON_GREYSCALE			GLSL_BIT(0)
#define GLSL_SHADER_COMMON_FOG					GLSL_BIT(1)
#define GLSL_SHADER_COMMON_FOG_RGB				GLSL_BIT(2)

#define GLSL_SHADER_COMMON_RGB_GEN_CONST 		GLSL_BIT(4)
#define GLSL_SHADER_COMMON_RGB_GEN_VERTEX 		GLSL_BIT(5)
#define GLSL_SHADER_COMMON_RGB_GEN_ONE_MINUS_VERTEX (GLSL_SHADER_COMMON_RGB_GEN_CONST | GLSL_SHADER_COMMON_RGB_GEN_VERTEX)
#define GLSL_SHADER_COMMON_RGB_DISTANCERAMP      GLSL_BIT(6)
#define GLSL_SHADER_COMMON_RGB_GEN_DIFFUSELIGHT  GLSL_BIT(7)

#define GLSL_SHADER_COMMON_ALPHA_GEN_CONST 		GLSL_BIT(8)
#define GLSL_SHADER_COMMON_ALPHA_GEN_VERTEX 	GLSL_BIT(9)
#define GLSL_SHADER_COMMON_ALPHA_GEN_ONE_MINUS_VERTEX (GLSL_SHADER_COMMON_ALPHA_GEN_CONST | \
												GLSL_SHADER_COMMON_ALPHA_GEN_VERTEX)
#define GLSL_SHADER_COMMON_ALPHA_DISTANCERAMP    GLSL_BIT(10)

#define GLSL_SHADER_COMMON_BONE_TRANSFORMS1		GLSL_BIT(11)
#define GLSL_SHADER_COMMON_BONE_TRANSFORMS2		GLSL_BIT(12)
#define GLSL_SHADER_COMMON_BONE_TRANSFORMS3		(GLSL_SHADER_COMMON_BONE_TRANSFORMS1 | GLSL_SHADER_COMMON_BONE_TRANSFORMS2)
#define GLSL_SHADER_COMMON_BONE_TRANSFORMS4		GLSL_BIT(13)
#define GLSL_SHADER_COMMON_BONE_TRANSFORMS		(GLSL_SHADER_COMMON_BONE_TRANSFORMS1 | GLSL_SHADER_COMMON_BONE_TRANSFORMS2 \
													| GLSL_SHADER_COMMON_BONE_TRANSFORMS3 | GLSL_SHADER_COMMON_BONE_TRANSFORMS4)

#define GLSL_SHADER_COMMON_DLIGHTS_4			GLSL_BIT(14) // 4
#define GLSL_SHADER_COMMON_DLIGHTS_8			GLSL_BIT(15) // 8
#define GLSL_SHADER_COMMON_DLIGHTS_12			(GLSL_SHADER_COMMON_DLIGHTS_4 | GLSL_SHADER_COMMON_DLIGHTS_8) // 12
#define GLSL_SHADER_COMMON_DLIGHTS_16			GLSL_BIT(16) // 16
#define GLSL_SHADER_COMMON_DLIGHTS				(GLSL_SHADER_COMMON_DLIGHTS_4 | GLSL_SHADER_COMMON_DLIGHTS_8 \
													| GLSL_SHADER_COMMON_DLIGHTS_12 | GLSL_SHADER_COMMON_DLIGHTS_16)

#define GLSL_SHADER_COMMON_DRAWFLAT				GLSL_BIT(17)

#define GLSL_SHADER_COMMON_AUTOSPRITE			GLSL_BIT(18)
#define GLSL_SHADER_COMMON_AUTOSPRITE2			GLSL_BIT(19)
#define GLSL_SHADER_COMMON_AUTOPARTICLE			GLSL_BIT(20)

#define GLSL_SHADER_COMMON_INSTANCED_TRANSFORMS	GLSL_BIT(21)
#define GLSL_SHADER_COMMON_INSTANCED_ATTRIB_TRANSFORMS	GLSL_BIT(22)

#define GLSL_SHADER_COMMON_SOFT_PARTICLE		GLSL_BIT(23)

#define GLSL_SHADER_COMMON_AFUNC_GT0			GLSL_BIT(24)
#define GLSL_SHADER_COMMON_AFUNC_LT128			GLSL_BIT(25)
#define GLSL_SHADER_COMMON_AFUNC_GE128			(GLSL_SHADER_COMMON_AFUNC_GT0 | GLSL_SHADER_COMMON_AFUNC_LT128)

#define GLSL_SHADER_COMMON_FRAGMENT_HIGHP		GLSL_BIT(26)

#define GLSL_SHADER_COMMON_TC_MOD				GLSL_BIT(27)

// material prgoram type features
#define GLSL_SHADER_MATERIAL_LIGHTSTYLE0		GLSL_BIT(32)
#define GLSL_SHADER_MATERIAL_LIGHTSTYLE1		GLSL_BIT(33)
#define GLSL_SHADER_MATERIAL_LIGHTSTYLE2		(GLSL_SHADER_MATERIAL_LIGHTSTYLE0 | GLSL_SHADER_MATERIAL_LIGHTSTYLE1)
#define GLSL_SHADER_MATERIAL_LIGHTSTYLE3		GLSL_BIT(34)
#define GLSL_SHADER_MATERIAL_LIGHTSTYLE				((GLSL_SHADER_MATERIAL_LIGHTSTYLE0 | GLSL_SHADER_MATERIAL_LIGHTSTYLE1 \
													| GLSL_SHADER_MATERIAL_LIGHTSTYLE2 | GLSL_SHADER_MATERIAL_LIGHTSTYLE3))
#define GLSL_SHADER_MATERIAL_SPECULAR			GLSL_BIT(35)
#define GLSL_SHADER_MATERIAL_DIRECTIONAL_LIGHT	GLSL_BIT(36)
#define GLSL_SHADER_MATERIAL_FB_LIGHTMAP		GLSL_BIT(37)
#define GLSL_SHADER_MATERIAL_OFFSETMAPPING		GLSL_BIT(38)
#define GLSL_SHADER_MATERIAL_RELIEFMAPPING		GLSL_BIT(39)
#define GLSL_SHADER_MATERIAL_AMBIENT_COMPENSATION GLSL_BIT(40)
#define GLSL_SHADER_MATERIAL_DECAL				GLSL_BIT(41)
#define GLSL_SHADER_MATERIAL_DECAL_ADD			GLSL_BIT(42)
#define GLSL_SHADER_MATERIAL_BASETEX_ALPHA_ONLY	GLSL_BIT(43)
#define GLSL_SHADER_MATERIAL_CELSHADING			GLSL_BIT(44)
#define GLSL_SHADER_MATERIAL_HALFLAMBERT		GLSL_BIT(45)
#define GLSL_SHADER_MATERIAL_DIRECTIONAL_LIGHT_MIX GLSL_BIT(46)
#define GLSL_SHADER_MATERIAL_ENTITY_DECAL		GLSL_BIT(47)
#define GLSL_SHADER_MATERIAL_ENTITY_DECAL_ADD	GLSL_BIT(48)
#define GLSL_SHADER_MATERIAL_DIRECTIONAL_LIGHT_FROM_NORMAL	GLSL_BIT(49)
#define GLSL_SHADER_MATERIAL_LIGHTMAP_ARRAYS	GLSL_BIT(50)
#define GLSL_SHADER_MATERIAL_CAMERA_AMBIENT_FILL	GLSL_BIT(51)

// q3a shader features
#define GLSL_SHADER_Q3_TC_GEN_ENV				GLSL_BIT(32)
#define GLSL_SHADER_Q3_TC_GEN_VECTOR			GLSL_BIT(33)
#define GLSL_SHADER_Q3_TC_GEN_REFLECTION		(GLSL_SHADER_Q3_TC_GEN_ENV | GLSL_SHADER_Q3_TC_GEN_VECTOR)
#define GLSL_SHADER_Q3_TC_GEN_CELSHADE			GLSL_BIT(34)
#define GLSL_SHADER_Q3_TC_GEN_PROJECTION		GLSL_BIT(35)
#define GLSL_SHADER_Q3_TC_GEN_SURROUND			GLSL_BIT(36)
#define GLSL_SHADER_Q3_COLOR_FOG				GLSL_BIT(37)
#define GLSL_SHADER_Q3_LIGHTSTYLE0				GLSL_BIT(38)
#define GLSL_SHADER_Q3_LIGHTSTYLE1				GLSL_BIT(39)
#define GLSL_SHADER_Q3_LIGHTSTYLE2				(GLSL_SHADER_Q3_LIGHTSTYLE0 | GLSL_SHADER_Q3_LIGHTSTYLE1)
#define GLSL_SHADER_Q3_LIGHTSTYLE3				GLSL_BIT(40)
#define GLSL_SHADER_Q3_LIGHTSTYLE				((GLSL_SHADER_Q3_LIGHTSTYLE0 | GLSL_SHADER_Q3_LIGHTSTYLE1 \
													| GLSL_SHADER_Q3_LIGHTSTYLE2 | GLSL_SHADER_Q3_LIGHTSTYLE3))
#define GLSL_SHADER_Q3_LIGHTMAP_ARRAYS			GLSL_BIT(41)
#define GLSL_SHADER_Q3_ALPHA_MASK				GLSL_BIT(42)

// distortions
#define GLSL_SHADER_DISTORTION_DUDV				GLSL_BIT(32)
#define GLSL_SHADER_DISTORTION_EYEDOT			GLSL_BIT(33)
#define GLSL_SHADER_DISTORTION_DISTORTION_ALPHA	GLSL_BIT(34)
#define GLSL_SHADER_DISTORTION_REFLECTION		GLSL_BIT(35)
#define GLSL_SHADER_DISTORTION_REFRACTION		GLSL_BIT(36)

// rgb shadows
#define GLSL_SHADER_RGBSHADOW_24BIT				GLSL_BIT(32)

// shadowmaps
#define GLSL_SHADOWMAP_LIMIT					4 // shadowmaps per program limit

// Matches `uniform texture2D lightmapTexture[16]` in defaultQ3AShader.frag.glsl and
// defaultMaterial.frag.glsl. Callers binding lightmaps by registerOffset must clamp to this;
// RP_BindDescriptorSets enforces the shader's reflected count as a backstop either way.
#define GLSL_LIGHTMAP_LIMIT					16

#define GLSL_SHADER_SHADOWMAP_DITHER			GLSL_BIT(32)
#define GLSL_SHADER_SHADOWMAP_PCF				GLSL_BIT(33)
#define GLSL_SHADER_SHADOWMAP_SHADOW2			GLSL_BIT(34)
#define GLSL_SHADER_SHADOWMAP_SHADOW3			GLSL_BIT(35)
#define GLSL_SHADER_SHADOWMAP_SHADOW4			GLSL_BIT(36)
#define GLSL_SHADER_SHADOWMAP_SAMPLERS			GLSL_BIT(37)
#define GLSL_SHADER_SHADOWMAP_24BIT				GLSL_BIT(38)
#define GLSL_SHADER_SHADOWMAP_NORMALCHECK		GLSL_BIT(39)

// outlines
#define GLSL_SHADER_OUTLINE_OUTLINES_CUTOFF		GLSL_BIT(32)

// cellshading program features
#define GLSL_SHADER_CELSHADE_DIFFUSE			GLSL_BIT(32)
#define GLSL_SHADER_CELSHADE_DECAL				GLSL_BIT(33)
#define GLSL_SHADER_CELSHADE_DECAL_ADD			GLSL_BIT(34)
#define GLSL_SHADER_CELSHADE_ENTITY_DECAL		GLSL_BIT(35)
#define GLSL_SHADER_CELSHADE_ENTITY_DECAL_ADD	GLSL_BIT(36)
#define GLSL_SHADER_CELSHADE_STRIPES			GLSL_BIT(37)
#define GLSL_SHADER_CELSHADE_STRIPES_ADD		GLSL_BIT(38)
#define GLSL_SHADER_CELSHADE_CEL_LIGHT			GLSL_BIT(39)
#define GLSL_SHADER_CELSHADE_CEL_LIGHT_ADD		GLSL_BIT(40)

// fxaa
#define GLSL_SHADER_FXAA_FXAA3					GLSL_BIT(32)

typedef enum {
	GLSL_STAGE_VERTEX,
	GLSL_STAGE_FRAGMENT,

	GLSL_STAGE_MAX
} glsl_program_stage_t;

struct glsl_program_descriptor_s {
		union {
#if ( DEVICE_IMPL_VULKAN )
			struct {
				VkDescriptorSetLayout setLayout;
			} vk;
#endif
#if ( DEVICE_IMPL_MTL )
			struct {
				// Metal argument-buffer sets have no layout object; what a set needs instead is where its
				// encoded table lives. `stride` is one slot (every stage's table back to back, each
				// aligned); stageOffset/stageLength carve it up. stageLength 0 means that stage does not
				// use this set -- and that is the only safe liveness test, because
				// newArgumentEncoderWithBufferIndex: aborts rather than returning nil for an index the
				// function does not declare.
				uint32_t stride;
				uint32_t stageOffset[GLSL_STAGE_MAX];
				uint32_t stageLength[GLSL_STAGE_MAX];
				uint8_t argBufferIndex; // MSL buffer index the table binds at; R_MTL_SLOT_UNUSED == discrete
			} mtl;
#endif
		};
		struct DescriptorSetAllocator alloc;
		uint16_t samplerMaxNum;
		uint16_t constantBufferMaxNum;
		uint16_t dynamicConstantBufferMaxNum;
		uint16_t textureMaxNum;
		uint16_t storageTextureMaxNum;
		uint16_t bufferMaxNum;
		uint16_t storageBufferMaxNum;
		uint16_t structuredBufferMaxNum;
		uint16_t storageStructuredBufferMaxNum;
		uint16_t accelerationStructureMaxNum;
};

struct glsl_program_s {
	union {
		struct {
#if ( DEVICE_IMPL_VULKAN )
			struct {
				VkShaderStageFlags shaderStageFlags;
				uint32_t size;
			} pushConstant;
			VkPipelineLayout pipelineLayout;
#endif
			uint32_t reserved; // keeps the arm non-empty on backends without a pipeline-layout object
		} vk;
#if ( DEVICE_IMPL_MTL )
		struct {
			// Metal binds resources directly; there is no pipeline-layout object. The push-constant block
			// is uploaded inline with setVertex/FragmentBytes, so all that is needed here is its size and
			// the MSL buffer index SPIRV-Cross gave it in each stage (R_MTL_SLOT_UNUSED = not declared).
			uint32_t pushConstantSize;
			uint8_t pushConstantSlot[GLSL_STAGE_MAX];
		} mtl;
#endif
	};

	char *name;
	int type;
	bool hasPushConstant;
	r_glslfeat_t features;
	char *deformsKey;
	struct glsl_program_s *hash_next;

	uint32_t vertexInputMask;
	struct shader_bin_data_s {
		char *bin;
		size_t size;
		glsl_program_stage_t stage;
	} shaderBin[GLSL_STAGE_MAX];

#if ( DEVICE_IMPL_MTL )
	// SPIR-V is cross-compiled to MSL (SPIRV-Cross) once at registration; the resulting MTLLibrary and
	// its "main0" entry point are kept per stage and referenced by every pipeline built from this program.
	// `msl` is retained purely for diagnostics (r_shaderdump) and is not needed after the library is built.
	struct shader_mtl_stage_s {
		struct mtlc_library lib;
		struct mtlc_function fn;
		char *msl;
		// Fragment stage only. The same SPIR-V cross-compiled with SPIRV-Cross's enable_frag_output_mask
		// = 0, so main0 declares no [[color(n)]] outputs. Vulkan silently discards writes to an attachment
		// the render pass does not have; Metal refuses to build a pipeline whose fragment function writes
		// color(0) while colorAttachments[0].pixelFormat is Invalid, which is exactly what every depth-only
		// pass (shadow maps) asks for. Nil when the stage has no colour outputs to strip.
		struct mtlc_library depthOnlyLib;
		struct mtlc_function depthOnlyFn;
		char *depthOnlyMsl; // diagnostics only, like `msl`
		// One MTLArgumentEncoder per (stage, set). Each stage is a separate cross-compile, so the same
		// descriptor set can be laid out differently in the vertex and fragment modules and needs its own
		// encoder. Vended from the function, hence caller-owned -- released alongside fn/lib.
		struct mtlc_argument_encoder argEncoder[R_DESCRIPTOR_SET_MAX];
	} mtlStage[GLSL_STAGE_MAX];
#endif
	
	struct pipeline_hash_s {
		uint64_t hash;
		union {
#if ( DEVICE_IMPL_VULKAN )
			struct {
				VkPipeline handle;
			} vk;
#endif
#if ( DEVICE_IMPL_MTL )
			struct {
				void *state; // MTLRenderPipelineState (built from SPIRV-Cross MSL)
			} mtl;
#endif
		};
	} pipelines[PIPELINE_LAYOUT_HASH_SIZE];

	struct glsl_program_descriptor_s programDescriptors[R_DESCRIPTOR_SET_MAX];

	size_t numDescriptorReflections;
	struct descriptor_reflection_s {
		hash_t hash;
		uint16_t isArray : 1;
		uint16_t dimCount : 8;
		uint16_t set : 3;
		uint16_t baseRegisterIndex;
#if ( DEVICE_IMPL_MTL )
		// Direct-binding model: MSL gives each stage its own buffer/texture/sampler index space, so a
		// Vulkan (set, binding) pair resolves to one slot per stage per resource class. The slots are
		// pinned at cross-compile time via spvc_compiler_msl_add_resource_binding_2, so these indices
		// are exactly what the encoder must bind to. 0xFF means "not used by that stage".
		uint8_t mtlSlot[GLSL_STAGE_MAX];   // MSL index within the class below
		uint8_t mtlClass;                  // enum r_mtl_bind_class_e
		uint8_t mtlTexDim;                 // enum r_mtl_tex_dim_e; only meaningful for R_MTL_BIND_TEXTURE
#endif
	} descriptorReflection[PIPELINE_REFLECTION_HASH_SIZE];
};

struct glsl_descriptor_handle_s {
	const char* name;
	hash_t hash;
};

// SPIRV-Reflect can report a NULL binding name (it guards for that itself), so strlen() here would
// crash on an anonymous uniform block. A nameless descriptor gets hash 0, which callers can never
// produce for a real handle and which the reflection builders treat as "not addressable by name".
static inline struct glsl_descriptor_handle_s Create_DescriptorHandle(const char* name) {
	const size_t len = name ? strlen(name) : 0;
	return (struct glsl_descriptor_handle_s){
		.name = name,
		.hash = len ? hash_data(HASH_INITIAL_VALUE, name, len) : 0
	};
}

struct glsl_descriptor_binding_s {
	struct glsl_descriptor_handle_s handle;
	uint32_t registerOffset; 
	struct RIDescriptor_s descriptor;
};

// we only support on push constant set
void RP_BindPushConstant(struct RIDevice_s *device, struct FrameState_s *cmd, struct glsl_program_s *program, void* data, size_t len);
void RP_BindDescriptorSets( struct RIDevice_s *device, struct FrameState_s *cmd, struct glsl_program_s *program, struct glsl_descriptor_binding_s *data, size_t numDescriptorData );

void RP_Init( void );
void RP_Shutdown( void );
void RP_PrecachePrograms( void );
void RP_StorePrecacheList( void );

void RP_ProgramList_f( void );

struct pipeline_hash_s *RP_ResolvePipeline( struct glsl_program_s *program, struct pipeline_desc_s* cmd);
void RP_BindPipeline( struct FrameState_s *cmd, struct pipeline_hash_s *pipeline );

struct glsl_program_s *RP_ResolveProgram( int type, const char *name, const char *deformsKey, const deformv_t *deforms, int numDeforms, r_glslfeat_t features );
struct glsl_program_s *RP_RegisterProgram( int type, const char *name, const char *deformsKey, const deformv_t *deforms, int numDeforms, r_glslfeat_t features );

bool RP_ProgramHasUniform(const struct glsl_program_s *program,const struct glsl_descriptor_handle_s handle );

#endif // R_PROGRAM_H
