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
#include "r_nri.h"
#include "r_vattribs.h"
#include "r_resource.h"

#include "r_descriptor_pool.h"
#include "ri_types.h"

#include "qhash.h"

#define GLSL_BIT(x)							(1ULL << (x))
#define GLSL_BITS_VERSION					16

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
		};
		struct descriptor_set_allloc_s alloc; // the set allocator 
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
		} vk;
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
	
	//NriPipelineLayout *layout;
	struct pipeline_hash_s {
		uint64_t hash;
		union {
#if ( DEVICE_IMPL_VULKAN )
			struct {
				VkPipeline handle;	
			} vk;
#endif
		};
	} pipelines[PIPELINE_LAYOUT_HASH_SIZE];

	struct glsl_program_descriptor_s programDescriptors[DESCRIPTOR_SET_MAX];

	size_t numDescriptorReflections;
	struct descriptor_reflection_s {
		hash_t hash;
		uint16_t isArray : 1;
		uint16_t dimCount : 8;
		uint16_t set : 3;
		uint16_t baseRegisterIndex;
		//uint16_t rangeOffset;
	} descriptorReflection[PIPELINE_REFLECTION_HASH_SIZE];
};

struct glsl_descriptor_handle_s {
	const char* name;
	hash_t hash;
};

static inline struct glsl_descriptor_handle_s Create_DescriptorHandle(const char* name) {
	return (struct glsl_descriptor_handle_s){
		.name = name,
		.hash = hash_data(HASH_INITIAL_VALUE, name, strlen(name))
	};
}

struct glsl_descriptor_binding_s {
	struct glsl_descriptor_handle_s handle;
	uint32_t registerOffset; 
	struct RIDescriptor_s descriptor;
};

// we only support on push constant set
void RP_BindPushConstant(struct RIDevice_s *device, struct frame_cmd_buffer_s *cmd, struct glsl_program_s *program, void* data, size_t len);
void RP_BindDescriptorSets( struct RIDevice_s *device, struct frame_cmd_buffer_s *cmd, struct glsl_program_s *program, struct glsl_descriptor_binding_s *data, size_t numDescriptorData );

void RP_Init( void );
void RP_Shutdown( void );
void RP_PrecachePrograms( void );
void RP_StorePrecacheList( void );

void RP_ProgramList_f( void );

struct pipeline_hash_s *RP_ResolvePipeline( struct glsl_program_s *program, struct pipeline_desc_s* cmd);
void RP_BindPipeline( struct frame_cmd_buffer_s *cmd, struct pipeline_hash_s *pipeline );

struct glsl_program_s *RP_ResolveProgram( int type, const char *name, const char *deformsKey, const deformv_t *deforms, int numDeforms, r_glslfeat_t features );
struct glsl_program_s *RP_RegisterProgram( int type, const char *name, const char *deformsKey, const deformv_t *deforms, int numDeforms, r_glslfeat_t features );

bool RP_ProgramHasUniform(const struct glsl_program_s *program,const struct glsl_descriptor_handle_s handle );

#endif // R_PROGRAM_H
