/*
Copyright (C) 2007 Victor Luchits

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

// r_program.c - OpenGL Shading Language support

#include "../qalgo/q_trie.h"
#include "qtypes.h"
#include "r_descriptor_pool.h"
#include "r_local.h"
#include "ri_vk.h"

#include "spirv_reflect.h"
#if ( DEVICE_IMPL_MTL )
	#include <spirv_cross_c.h>
#endif
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Include/glslang_c_shader_types.h>
#include <glslang/Public/resource_limits_c.h>

#include "r_vattribs.h"
#include "tracy/TracyC.h"

#include "ri_conversion.h"
#include "ri_conversion_mtl.h"
#include "ri_mtl.h"
#include "ri_renderer.h"

#include "../../gameshared/q_arch.h"
#include "qhash.h"
#include "qstr.h"
#include "stb_ds.h"

#define MAX_GLSL_PROGRAMS 1024
#define GLSL_PROGRAMS_HASH_SIZE 256

typedef struct {
	r_glslfeat_t bit;
	const char *define;
	const char *suffix;
} glsl_feature_t;

trie_t *glsl_cache_trie = NULL;

static unsigned int r_numglslprograms;
static struct glsl_program_s r_glslprograms[MAX_GLSL_PROGRAMS];
static struct glsl_program_s *r_glslprograms_hash[GLSL_PROGRAM_TYPE_MAXTYPE][GLSL_PROGRAMS_HASH_SIZE];

static glslang_stage_t __RP_GLStageToSlang( glsl_program_stage_t stage )
{
	switch( stage ) {
		case GLSL_STAGE_VERTEX:
			return GLSLANG_STAGE_VERTEX;
		case GLSL_STAGE_FRAGMENT:
			return GLSLANG_STAGE_FRAGMENT;
		default:
			break;
	}
	// unhandled return invalid stage
	assert( 0 );
	return GLSLANG_STAGE_COUNT;
}

// static struct ProgramDescriptorInfo* __GetDescriptorInfo(struct glsl_program_s* program, uint32_t set) {
//	for( size_t i = 0; i < program->numSets; i++ ) {
//		if( program->programDescriptors[i].registerSpace == set ) {
//			return &program->programDescriptors[i];
//		}
//	}
//	return NULL;
// }

/*
 * RP_Init
 */
void RP_Init( void )
{
	TracyCZoneN( ctx, "RP_Init", 1 );
	glslang_initialize_process();
	memset( r_glslprograms, 0, sizeof( r_glslprograms ) );
	memset( r_glslprograms_hash, 0, sizeof( r_glslprograms_hash ) );

	Trie_Create( TRIE_CASE_INSENSITIVE, &glsl_cache_trie );

	// register base programs
	RP_RegisterProgram( GLSL_PROGRAM_TYPE_MATERIAL, DEFAULT_GLSL_MATERIAL_PROGRAM, NULL, NULL, 0, 0 );
	RP_RegisterProgram( GLSL_PROGRAM_TYPE_DISTORTION, DEFAULT_GLSL_DISTORTION_PROGRAM, NULL, NULL, 0, 0 );
	RP_RegisterProgram( GLSL_PROGRAM_TYPE_RGB_SHADOW, DEFAULT_GLSL_RGB_SHADOW_PROGRAM, NULL, NULL, 0, 0 );
	RP_RegisterProgram( GLSL_PROGRAM_TYPE_SHADOWMAP, DEFAULT_GLSL_SHADOWMAP_PROGRAM, NULL, NULL, 0, 0 );
	RP_RegisterProgram( GLSL_PROGRAM_TYPE_OUTLINE, DEFAULT_GLSL_OUTLINE_PROGRAM, NULL, NULL, 0, 0 );
	RP_RegisterProgram( GLSL_PROGRAM_TYPE_Q3A_SHADER, DEFAULT_GLSL_Q3A_SHADER_PROGRAM, NULL, NULL, 0, 0 );
	RP_RegisterProgram( GLSL_PROGRAM_TYPE_CELSHADE, DEFAULT_GLSL_CELSHADE_PROGRAM, NULL, NULL, 0, 0 );
	RP_RegisterProgram( GLSL_PROGRAM_TYPE_FOG, DEFAULT_GLSL_FOG_PROGRAM, NULL, NULL, 0, 0 );
	RP_RegisterProgram( GLSL_PROGRAM_TYPE_FXAA, DEFAULT_GLSL_FXAA_PROGRAM, NULL, NULL, 0, 0 );
	RP_RegisterProgram( GLSL_PROGRAM_TYPE_YUV, DEFAULT_GLSL_YUV_PROGRAM, NULL, NULL, 0, 0 );
	RP_RegisterProgram( GLSL_PROGRAM_TYPE_COLORCORRECTION, DEFAULT_GLSL_COLORCORRECTION_PROGRAM, NULL, NULL, 0, 0 );
	// check whether compilation of the shader with GPU skinning succeeds, if not, disable GPU bone transforms
	RP_RegisterProgram( GLSL_PROGRAM_TYPE_MATERIAL, DEFAULT_GLSL_MATERIAL_PROGRAM, NULL, NULL, 0, GLSL_SHADER_COMMON_BONE_TRANSFORMS1 );
	TracyCZoneEnd( ctx );
}

typedef struct {
	const glsl_feature_t *it;
	r_glslfeat_t bits;
} feature_iter_t;

static bool __R_IsValidFeatureIter( feature_iter_t *iter )
{
	return iter->it != NULL;
}

static feature_iter_t __R_NextFeature( feature_iter_t iter )
{
	for( ; iter.it && iter.bits > 0 && iter.it->bit > 0; iter.it++ ) {
		if( ( iter.bits & iter.it->bit ) == iter.it->bit ) {
			iter.bits &= ~iter.it->bit;
			return iter;
		}
	}
	iter.it = NULL;
	return iter;
}

static void __appendGLSLDeformv( struct QStr *str, const deformv_t *deformv, int numDeforms )
{
	static const char *const funcs[] = { NULL, "WAVE_SIN", "WAVE_TRIANGLE", "WAVE_SQUARE", "WAVE_SAWTOOTH", "WAVE_INVERSESAWTOOTH", NULL };
	static const int numSupportedFuncs = sizeof( funcs ) / sizeof( funcs[0] ) - 1;
	if( !numDeforms ) {
		return;
	}
	const int funcType = deformv->func.type;
	for( size_t i = 0; i < numDeforms; i++, deformv++ ) {
		switch( deformv->type ) {
			case DEFORMV_WAVE: {
				if( funcType <= SHADER_FUNC_NONE || funcType > numSupportedFuncs || !funcs[funcType] )
					break;
				qStrAppendSlice( str, qCToStrRef( "#define DEFORMV_WAVE 1 \n" ) );
				qstrcatfmt( str, "#define DEFORMV_WAVE_FUNC %s \n", funcs[funcType] );
				qstrcatprintf( str, "#define DEFORMV_WAVE_CONSTANT vec4(%f,%f,%f,%f) \n",
							   deformv->func.args[0],	// base
							   deformv->func.args[1],	// amplitude
							   deformv->func.args[2],	// phase
							   deformv->func.args[3] ); // frequency
				qstrcatprintf( str, "#define DEFORMV_WAVE_DIVISOR %f \n",
							   deformv->args[0] ); // spatial divisor (1.0 / wavelength)
				break;
			}
			case DEFORMV_MOVE: {
				if( funcType <= SHADER_FUNC_NONE || funcType > numSupportedFuncs || !funcs[funcType] )
					break;
				qStrAppendSlice( str, qCToStrRef( "#define DEFORMV_MOVE 1 \n" ) );
				qstrcatfmt( str, "#define DEFORMV_MOVE_FUNC %s \n", funcs[funcType] );
				qstrcatprintf( str, "#define DEFORMV_MOVE_CONSTANT vec4(%f,%f,%f,%f) \n",
							   deformv->func.args[0],	// base
							   deformv->func.args[1],	// amplitude
							   deformv->func.args[2],	// phase
							   deformv->func.args[3] ); // frequency
				qstrcatprintf( str, "#define DEFORMV_MOVE_DIRECTION vec3(%f,%f,%f) \n",
							   deformv->args[0],   // move x
							   deformv->args[1],   // move y
							   deformv->args[2] ); // move z
				break;
			}
			case DEFORMV_BULGE:
				qStrAppendSlice( str, qCToStrRef( "#define DEFORMV_BULGE 1 \n" ) );
				qstrcatprintf( str, "#define DEFORMV_BULGE_CONSTANT vec4(%f,%f,%f,%f) \n",
							   deformv->args[0],   // width
							   deformv->args[1],   // height
							   deformv->args[2],   // speed
							   deformv->args[3] ); // phase
				break;
			case DEFORMV_AUTOSPRITE:
				qStrAppendSlice( str, qCToStrRef( "#define DEFORMV_AUTOSPRITE 1 \n" ) );
				break;
			case DEFORMV_AUTOPARTICLE:
				qStrAppendSlice( str, qCToStrRef( "#define DEFORMV_AUTOPARTICLE 1 \n" ) );
				break;
			case DEFORMV_AUTOSPRITE2:
				qStrAppendSlice( str, qCToStrRef( "#define DEFORMV_AUTOSPRITE2 1 \n" ) );
				break;
			default:
				break;
		}
	}
}

/*
 * RP_PrecachePrograms
 *
 * Loads the list of known program permutations from disk file.
 *
 * Expected file format:
 * application_name\n
 * version_number\n*
 * program_type1 features_lower_bits1 features_higher_bits1 program_name1 binary_offset
 * ..
 * program_typeN features_lower_bitsN features_higher_bitsN program_nameN binary_offset
 */
void RP_PrecachePrograms( void ) {}

/*
 * RP_StorePrecacheList
 *
 * Stores the list of known GLSL program permutations to file on the disk.
 * File format matches that expected by RP_PrecachePrograms.
 */
void RP_StorePrecacheList( void ) {}

/*
 * RF_DeleteProgram
 */
static void RF_DeleteProgram( struct glsl_program_s *program )
{
	struct glsl_program_s *hash_next;
	TracyCZoneN( ctx, "RF_DeleteProgram", 1 );

	if( program->name )
		R_Free( program->name );
	if( program->deformsKey )
		R_Free( program->deformsKey );

#if ( DEVICE_IMPL_VULKAN )
	if( program->vk.pipelineLayout ) {
		vkDestroyPipelineLayout( rsh.device.vk.device, program->vk.pipelineLayout, NULL );
	}
	for( size_t i = 0; i < PIPELINE_LAYOUT_HASH_SIZE; i++ ) {
		if( program->pipelines[i].vk.handle )
			vkDestroyPipeline( rsh.device.vk.device, program->pipelines[i].vk.handle, NULL );
	}
	for( size_t i = 0; i < R_DESCRIPTOR_SET_MAX; i++ ) {
		if( program->programDescriptors[i].vk.setLayout ) {
			vkDestroyDescriptorSetLayout( rsh.device.vk.device, program->programDescriptors[i].vk.setLayout, NULL );
			FreeDescriptorSetAlloc( &rsh.device, &program->programDescriptors[i].alloc );
		}
	}
#endif
#if ( DEVICE_IMPL_MTL )
	// Everything below came from a metal-c *_new_* call and is caller-owned; without this a vid_restart
	// leaks the whole pipeline cache plus one MTLLibrary/MTLFunction pair per stage.
	for( size_t i = 0; i < PIPELINE_LAYOUT_HASH_SIZE; i++ ) {
		if( program->pipelines[i].mtl.state )
			mtlc_render_pipeline_state_release( mtlc_render_pipeline_state_from_id( program->pipelines[i].mtl.state ) );
	}
	for( size_t i = 0; i < R_DESCRIPTOR_SET_MAX; i++ ) {
		if( program->programDescriptors[i].mtl.argBufferIndex != R_MTL_SLOT_UNUSED )
			FreeDescriptorSetAlloc( &rsh.device, &program->programDescriptors[i].alloc );
	}
	for( size_t i = 0; i < GLSL_STAGE_MAX; i++ ) {
		for( size_t set = 0; set < R_DESCRIPTOR_SET_MAX; set++ ) {
			if( !mtlc_argument_encoder_is_nil( program->mtlStage[i].argEncoder[set] ) )
				mtlc_argument_encoder_release( program->mtlStage[i].argEncoder[set] );
		}
		if( !mtlc_function_is_nil( program->mtlStage[i].fn ) )
			mtlc_function_release( program->mtlStage[i].fn );
		if( !mtlc_library_is_nil( program->mtlStage[i].lib ) )
			mtlc_library_release( program->mtlStage[i].lib );
		if( program->mtlStage[i].msl )
			R_Free( program->mtlStage[i].msl );
		if( !mtlc_function_is_nil( program->mtlStage[i].depthOnlyFn ) )
			mtlc_function_release( program->mtlStage[i].depthOnlyFn );
		if( !mtlc_library_is_nil( program->mtlStage[i].depthOnlyLib ) )
			mtlc_library_release( program->mtlStage[i].depthOnlyLib );
		if( program->mtlStage[i].depthOnlyMsl )
			R_Free( program->mtlStage[i].depthOnlyMsl );
	}
#endif
	for( size_t i = 0; i < GLSL_STAGE_MAX; i++ ) {
		if( program->shaderBin[i].bin )
			R_Free( program->shaderBin[i].bin );
	}

	hash_next = program->hash_next;
	memset( program, 0, sizeof( struct glsl_program_s ) );
	program->hash_next = hash_next;
	TracyCZoneEnd( ctx );
}

#define MAX_DEFINES_FEATURES 255

static const glsl_feature_t glsl_features_empty[] = { { 0, NULL, NULL } };

static const glsl_feature_t glsl_features_generic[] = { { GLSL_SHADER_COMMON_GREYSCALE, "#define APPLY_GREYSCALE\n", "_grey" },

														{ 0, NULL, NULL } };

static const glsl_feature_t glsl_features_material[] = { { GLSL_SHADER_COMMON_GREYSCALE, "#define APPLY_GREYSCALE\n", "_grey" },

														 { GLSL_SHADER_COMMON_BONE_TRANSFORMS4, "#define QF_NUM_BONE_INFLUENCES 4\n", "_bones4" },
														 { GLSL_SHADER_COMMON_BONE_TRANSFORMS3, "#define QF_NUM_BONE_INFLUENCES 3\n", "_bones3" },
														 { GLSL_SHADER_COMMON_BONE_TRANSFORMS2, "#define QF_NUM_BONE_INFLUENCES 2\n", "_bones2" },
														 { GLSL_SHADER_COMMON_BONE_TRANSFORMS1, "#define QF_NUM_BONE_INFLUENCES 1\n", "_bones1" },

														 { GLSL_SHADER_COMMON_RGB_GEN_ONE_MINUS_VERTEX, "#define APPLY_RGB_ONE_MINUS_VERTEX\n", "_c1-v" },
														 { GLSL_SHADER_COMMON_RGB_GEN_CONST, "#define APPLY_RGB_CONST\n", "_cc" },
														 { GLSL_SHADER_COMMON_RGB_GEN_VERTEX, "#define APPLY_RGB_VERTEX\n", "_cv" },
														 { GLSL_SHADER_COMMON_RGB_DISTANCERAMP, "#define APPLY_RGB_DISTANCERAMP\n", "_rgb_dr" },

														 { GLSL_SHADER_COMMON_ALPHA_GEN_ONE_MINUS_VERTEX, "#define APPLY_ALPHA_ONE_MINUS_VERTEX\n", "_a1-v" },
														 { GLSL_SHADER_COMMON_ALPHA_GEN_VERTEX, "#define APPLY_ALPHA_VERTEX\n", "_av" },
														 { GLSL_SHADER_COMMON_ALPHA_GEN_CONST, "#define APPLY_ALPHA_CONST\n", "_ac" },
														 { GLSL_SHADER_COMMON_ALPHA_DISTANCERAMP, "#define APPLY_ALPHA_DISTANCERAMP\n", "_alpha_dr" },

														 { GLSL_SHADER_COMMON_FOG, "#define APPLY_FOG\n#define APPLY_FOG_IN 1\n", "_fog" },
														 { GLSL_SHADER_COMMON_FOG_RGB, "#define APPLY_FOG_COLOR\n", "_rgb" },

														 { GLSL_SHADER_COMMON_DLIGHTS_16, "#define NUM_DLIGHTS 16\n", "_dl16" },
														 { GLSL_SHADER_COMMON_DLIGHTS_12, "#define NUM_DLIGHTS 12\n", "_dl12" },
														 { GLSL_SHADER_COMMON_DLIGHTS_8, "#define NUM_DLIGHTS 8\n", "_dl8" },
														 { GLSL_SHADER_COMMON_DLIGHTS_4, "#define NUM_DLIGHTS 4\n", "_dl4" },

														 { GLSL_SHADER_COMMON_DRAWFLAT, "#define APPLY_DRAWFLAT\n", "_flat" },

														 { GLSL_SHADER_COMMON_AUTOSPRITE, "#define APPLY_AUTOSPRITE\n", "" },
														 { GLSL_SHADER_COMMON_AUTOSPRITE2, "#define APPLY_AUTOSPRITE2\n", "" },
														 { GLSL_SHADER_COMMON_AUTOPARTICLE, "#define APPLY_AUTOSPRITE\n#define APPLY_AUTOPARTICLE\n", "" },

														 { GLSL_SHADER_COMMON_INSTANCED_TRANSFORMS, "#define APPLY_INSTANCED_TRANSFORMS\n", "_instanced" },
														 { GLSL_SHADER_COMMON_INSTANCED_ATTRIB_TRANSFORMS,
														   "#define APPLY_INSTANCED_TRANSFORMS\n"
														   "#define APPLY_INSTANCED_ATTRIB_TRANSFORMS\n",
														   "_instanced_va" },

														 { GLSL_SHADER_COMMON_AFUNC_GE128, "#define QF_ALPHATEST(a) { if ((a) < 0.5) discard; }\n", "_afunc_ge128" },
														 { GLSL_SHADER_COMMON_AFUNC_LT128, "#define QF_ALPHATEST(a) { if ((a) >= 0.5) discard; }\n", "_afunc_lt128" },
														 { GLSL_SHADER_COMMON_AFUNC_GT0, "#define QF_ALPHATEST(a) { if ((a) <= 0.0) discard; }\n", "_afunc_gt0" },

														 { GLSL_SHADER_COMMON_TC_MOD, "#define APPLY_TC_MOD\n", "_tc_mod" },

														 { GLSL_SHADER_MATERIAL_LIGHTSTYLE3, "#define NUM_LIGHTMAPS 4\n#define qf_lmvec01 vec4\n#define qf_lmvec23 vec4\n", "_ls3" },
														 { GLSL_SHADER_MATERIAL_LIGHTSTYLE2, "#define NUM_LIGHTMAPS 3\n#define qf_lmvec01 vec4\n#define qf_lmvec23 vec2\n", "_ls2" },
														 { GLSL_SHADER_MATERIAL_LIGHTSTYLE1, "#define NUM_LIGHTMAPS 2\n#define qf_lmvec01 vec4\n", "_ls1" },
														 { GLSL_SHADER_MATERIAL_LIGHTSTYLE0, "#define NUM_LIGHTMAPS 1\n#define qf_lmvec01 vec2\n", "_ls0" },
														 { GLSL_SHADER_MATERIAL_LIGHTMAP_ARRAYS, "#define LIGHTMAP_ARRAYS\n", "_lmarray" },
														 { GLSL_SHADER_MATERIAL_FB_LIGHTMAP, "#define APPLY_FBLIGHTMAP\n", "_fb" },
														 { GLSL_SHADER_MATERIAL_DIRECTIONAL_LIGHT, "#define APPLY_DIRECTIONAL_LIGHT\n", "_dirlight" },

														 { GLSL_SHADER_MATERIAL_SPECULAR, "#define APPLY_SPECULAR\n", "_gloss" },
														 { GLSL_SHADER_MATERIAL_OFFSETMAPPING, "#define APPLY_OFFSETMAPPING\n", "_offmap" },
														 { GLSL_SHADER_MATERIAL_RELIEFMAPPING, "#define APPLY_RELIEFMAPPING\n", "_relmap" },
														 { GLSL_SHADER_MATERIAL_AMBIENT_COMPENSATION, "#define APPLY_AMBIENT_COMPENSATION\n", "_amb" },
														 { GLSL_SHADER_MATERIAL_DECAL, "#define APPLY_DECAL\n", "_decal" },
														 { GLSL_SHADER_MATERIAL_DECAL_ADD, "#define APPLY_DECAL_ADD\n", "_add" },
														 { GLSL_SHADER_MATERIAL_BASETEX_ALPHA_ONLY, "#define APPLY_BASETEX_ALPHA_ONLY\n", "_alpha" },
														 { GLSL_SHADER_MATERIAL_CELSHADING, "#define APPLY_CELSHADING\n", "_cel" },
														 { GLSL_SHADER_MATERIAL_HALFLAMBERT, "#define APPLY_HALFLAMBERT\n", "_lambert" },

														 { GLSL_SHADER_MATERIAL_ENTITY_DECAL, "#define APPLY_ENTITY_DECAL\n", "_decal2" },
														 { GLSL_SHADER_MATERIAL_ENTITY_DECAL_ADD, "#define APPLY_ENTITY_DECAL_ADD\n", "_decal2_add" },

														 // doesn't make sense without APPLY_DIRECTIONAL_LIGHT
														 { GLSL_SHADER_MATERIAL_DIRECTIONAL_LIGHT_MIX, "#define APPLY_DIRECTIONAL_LIGHT_MIX\n", "_mix" },
														 { GLSL_SHADER_MATERIAL_DIRECTIONAL_LIGHT_FROM_NORMAL, "#define APPLY_DIRECTIONAL_LIGHT_FROM_NORMAL\n", "_normlight" },

														 { GLSL_SHADER_MATERIAL_CAMERA_AMBIENT_FILL, "#define APPLY_CAMERA_AMBIENT_FILL\n", "_camamb" },

														 { 0, NULL, NULL } };

static const glsl_feature_t glsl_features_distortion[] = { { GLSL_SHADER_COMMON_GREYSCALE, "#define APPLY_GREYSCALE\n", "_grey" },

														   { GLSL_SHADER_COMMON_RGB_GEN_ONE_MINUS_VERTEX, "#define APPLY_RGB_ONE_MINUS_VERTEX\n", "_c1-v" },
														   { GLSL_SHADER_COMMON_RGB_GEN_CONST, "#define APPLY_RGB_CONST\n", "_cc" },
														   { GLSL_SHADER_COMMON_RGB_GEN_VERTEX, "#define APPLY_RGB_VERTEX\n", "_cv" },
														   { GLSL_SHADER_COMMON_RGB_DISTANCERAMP, "#define APPLY_RGB_DISTANCERAMP\n", "_rgb_dr" },

														   { GLSL_SHADER_COMMON_ALPHA_GEN_ONE_MINUS_VERTEX, "#define APPLY_ALPHA_ONE_MINUS_VERTEX\n", "_a1-v" },
														   { GLSL_SHADER_COMMON_ALPHA_GEN_CONST, "#define APPLY_ALPHA_CONST\n", "_ac" },
														   { GLSL_SHADER_COMMON_ALPHA_GEN_VERTEX, "#define APPLY_ALPHA_VERTEX\n", "_av" },
														   { GLSL_SHADER_COMMON_ALPHA_DISTANCERAMP, "#define APPLY_ALPHA_DISTANCERAMP\n", "_alpha_dr" },

														   { GLSL_SHADER_COMMON_AUTOSPRITE, "#define APPLY_AUTOSPRITE\n", "" },
														   { GLSL_SHADER_COMMON_AUTOSPRITE2, "#define APPLY_AUTOSPRITE2\n", "" },
														   { GLSL_SHADER_COMMON_AUTOPARTICLE, "#define APPLY_AUTOSPRITE\n#define APPLY_AUTOPARTICLE\n", "" },

														   { GLSL_SHADER_COMMON_INSTANCED_TRANSFORMS, "#define APPLY_INSTANCED_TRANSFORMS\n", "_instanced" },
														   { GLSL_SHADER_COMMON_INSTANCED_ATTRIB_TRANSFORMS,
															 "#define APPLY_INSTANCED_TRANSFORMS\n"
															 "#define APPLY_INSTANCED_ATTRIB_TRANSFORMS\n",
															 "_instanced_va" },

														   { GLSL_SHADER_COMMON_FOG, "#define APPLY_FOG\n#define APPLY_FOG_IN 1\n", "_fog" },
														   { GLSL_SHADER_COMMON_FOG_RGB, "#define APPLY_FOG_COLOR\n", "_rgb" },

														   { GLSL_SHADER_DISTORTION_DUDV, "#define APPLY_DUDV\n", "_dudv" },
														   { GLSL_SHADER_DISTORTION_EYEDOT, "#define APPLY_EYEDOT\n", "_eyedot" },
														   { GLSL_SHADER_DISTORTION_DISTORTION_ALPHA, "#define APPLY_DISTORTION_ALPHA\n", "_alpha" },
														   { GLSL_SHADER_DISTORTION_REFLECTION, "#define APPLY_REFLECTION\n", "_refl" },
														   { GLSL_SHADER_DISTORTION_REFRACTION, "#define APPLY_REFRACTION\n", "_refr" },

														   { 0, NULL, NULL } };

static const glsl_feature_t glsl_features_rgbshadow[] = {
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS4, "#define QF_NUM_BONE_INFLUENCES 4\n", "_bones4" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS3, "#define QF_NUM_BONE_INFLUENCES 3\n", "_bones3" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS2, "#define QF_NUM_BONE_INFLUENCES 2\n", "_bones2" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS1, "#define QF_NUM_BONE_INFLUENCES 1\n", "_bones1" },

	{ GLSL_SHADER_COMMON_INSTANCED_TRANSFORMS, "#define APPLY_INSTANCED_TRANSFORMS\n", "_instanced" },
	{ GLSL_SHADER_COMMON_INSTANCED_ATTRIB_TRANSFORMS, "#define APPLY_INSTANCED_TRANSFORMS\n#define APPLY_INSTANCED_ATTRIB_TRANSFORMS\n", "_instanced_va" },

	{ GLSL_SHADER_RGBSHADOW_24BIT, "#define APPLY_RGB_SHADOW_24BIT\n", "_rgb24" },

	{ 0, NULL, NULL } };

static const glsl_feature_t glsl_features_shadowmap[] = {
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS4, "#define QF_NUM_BONE_INFLUENCES 4\n", "_bones4" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS3, "#define QF_NUM_BONE_INFLUENCES 3\n", "_bones3" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS2, "#define QF_NUM_BONE_INFLUENCES 2\n", "_bones2" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS1, "#define QF_NUM_BONE_INFLUENCES 1\n", "_bones1" },

	{ GLSL_SHADER_COMMON_INSTANCED_TRANSFORMS, "#define APPLY_INSTANCED_TRANSFORMS\n", "_instanced" },
	{ GLSL_SHADER_COMMON_INSTANCED_ATTRIB_TRANSFORMS, "#define APPLY_INSTANCED_TRANSFORMS\n#define APPLY_INSTANCED_ATTRIB_TRANSFORMS\n", "_instanced_va" },

	{ GLSL_SHADER_SHADOWMAP_DITHER, "#define APPLY_DITHER\n", "_dither" },
	{ GLSL_SHADER_SHADOWMAP_PCF, "#define APPLY_PCF\n", "_pcf" },
	{ GLSL_SHADER_SHADOWMAP_SHADOW2, "#define NUM_SHADOWS 2\n", "_2" },
	{ GLSL_SHADER_SHADOWMAP_SHADOW3, "#define NUM_SHADOWS 3\n", "_3" },
	{ GLSL_SHADER_SHADOWMAP_SHADOW4, "#define NUM_SHADOWS 4\n", "_4" },
	{ GLSL_SHADER_SHADOWMAP_SAMPLERS, "#define APPLY_SHADOW_SAMPLERS\n", "_shadowsamp" },
	{ GLSL_SHADER_SHADOWMAP_24BIT, "#define APPLY_RGB_SHADOW_24BIT\n", "_rgb24" },
	{ GLSL_SHADER_SHADOWMAP_NORMALCHECK, "#define APPLY_SHADOW_NORMAL_CHECK\n", "_nc" },

	{ 0, NULL, NULL } };

static const glsl_feature_t glsl_features_outline[] = {
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS4, "#define QF_NUM_BONE_INFLUENCES 4\n", "_bones4" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS3, "#define QF_NUM_BONE_INFLUENCES 3\n", "_bones3" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS2, "#define QF_NUM_BONE_INFLUENCES 2\n", "_bones2" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS1, "#define QF_NUM_BONE_INFLUENCES 1\n", "_bones1" },

	{ GLSL_SHADER_COMMON_RGB_GEN_CONST, "#define APPLY_RGB_CONST\n", "_cc" },
	{ GLSL_SHADER_COMMON_ALPHA_GEN_CONST, "#define APPLY_ALPHA_CONST\n", "_ac" },

	{ GLSL_SHADER_COMMON_FOG, "#define APPLY_FOG\n#define APPLY_FOG_IN 1\n", "_fog" },
	{ GLSL_SHADER_COMMON_FOG_RGB, "#define APPLY_FOG_COLOR\n", "_rgb" },

	{ GLSL_SHADER_COMMON_INSTANCED_TRANSFORMS, "#define APPLY_INSTANCED_TRANSFORMS\n", "_instanced" },
	{ GLSL_SHADER_COMMON_INSTANCED_ATTRIB_TRANSFORMS, "#define APPLY_INSTANCED_TRANSFORMS\n#define APPLY_INSTANCED_ATTRIB_TRANSFORMS\n", "_instanced_va" },

	{ GLSL_SHADER_OUTLINE_OUTLINES_CUTOFF, "#define APPLY_OUTLINES_CUTOFF\n", "_outcut" },

	{ 0, NULL, NULL } };

static const glsl_feature_t glsl_features_q3a[] = {
	{ GLSL_SHADER_COMMON_GREYSCALE, "#define APPLY_GREYSCALE\n", "_grey" },

	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS4, "#define QF_NUM_BONE_INFLUENCES 4\n", "_bones4" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS3, "#define QF_NUM_BONE_INFLUENCES 3\n", "_bones3" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS2, "#define QF_NUM_BONE_INFLUENCES 2\n", "_bones2" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS1, "#define QF_NUM_BONE_INFLUENCES 1\n", "_bones1" },

	{ GLSL_SHADER_COMMON_RGB_GEN_ONE_MINUS_VERTEX, "#define APPLY_RGB_ONE_MINUS_VERTEX\n", "_c1-v" },
	{ GLSL_SHADER_COMMON_RGB_GEN_CONST, "#define APPLY_RGB_CONST\n", "_cc" },
	{ GLSL_SHADER_COMMON_RGB_GEN_VERTEX, "#define APPLY_RGB_VERTEX\n", "_cv" },
	{ GLSL_SHADER_COMMON_RGB_DISTANCERAMP, "#define APPLY_RGB_DISTANCERAMP\n", "_rgb_dr" },

	{ GLSL_SHADER_COMMON_ALPHA_GEN_ONE_MINUS_VERTEX, "#define APPLY_ALPHA_ONE_MINUS_VERTEX\n", "_a1-v" },
	{ GLSL_SHADER_COMMON_ALPHA_GEN_CONST, "#define APPLY_ALPHA_CONST\n", "_ac" },
	{ GLSL_SHADER_COMMON_ALPHA_GEN_VERTEX, "#define APPLY_ALPHA_VERTEX\n", "_av" },
	{ GLSL_SHADER_COMMON_ALPHA_DISTANCERAMP, "#define APPLY_ALPHA_DISTANCERAMP\n", "_alpha_dr" },

	{ GLSL_SHADER_COMMON_FOG, "#define APPLY_FOG\n#define APPLY_FOG_IN 1\n", "_fog" },
	{ GLSL_SHADER_COMMON_FOG_RGB, "#define APPLY_FOG_COLOR\n", "_rgb" },

	{ GLSL_SHADER_COMMON_DLIGHTS_16, "#define NUM_DLIGHTS 16\n", "_dl16" },
	{ GLSL_SHADER_COMMON_DLIGHTS_12, "#define NUM_DLIGHTS 12\n", "_dl12" },
	{ GLSL_SHADER_COMMON_DLIGHTS_8, "#define NUM_DLIGHTS 8\n", "_dl8" },
	{ GLSL_SHADER_COMMON_DLIGHTS_4, "#define NUM_DLIGHTS 4\n", "_dl4" },

	{ GLSL_SHADER_COMMON_DRAWFLAT, "#define APPLY_DRAWFLAT\n", "_flat" },

	{ GLSL_SHADER_COMMON_AUTOSPRITE, "#define APPLY_AUTOSPRITE\n", "" },
	{ GLSL_SHADER_COMMON_AUTOSPRITE2, "#define APPLY_AUTOSPRITE2\n", "" },
	{ GLSL_SHADER_COMMON_AUTOPARTICLE, "#define APPLY_AUTOSPRITE\n#define APPLY_AUTOPARTICLE\n", "" },

	{ GLSL_SHADER_COMMON_INSTANCED_TRANSFORMS, "#define APPLY_INSTANCED_TRANSFORMS\n", "_instanced" },
	{ GLSL_SHADER_COMMON_INSTANCED_ATTRIB_TRANSFORMS, "#define APPLY_INSTANCED_TRANSFORMS\n#define APPLY_INSTANCED_ATTRIB_TRANSFORMS\n", "_instanced_va" },

	{ GLSL_SHADER_COMMON_SOFT_PARTICLE, "#define APPLY_SOFT_PARTICLE\n", "_sp" },

	{ GLSL_SHADER_COMMON_AFUNC_GE128, "#define QF_ALPHATEST(a) { if ((a) < 0.5) discard; }\n", "_afunc_ge128" },
	{ GLSL_SHADER_COMMON_AFUNC_LT128, "#define QF_ALPHATEST(a) { if ((a) >= 0.5) discard; }\n", "_afunc_lt128" },
	{ GLSL_SHADER_COMMON_AFUNC_GT0, "#define QF_ALPHATEST(a) { if ((a) <= 0.0) discard; }\n", "_afunc_gt0" },

	{ GLSL_SHADER_COMMON_TC_MOD, "#define APPLY_TC_MOD\n", "_tc_mod" },

	{ GLSL_SHADER_Q3_TC_GEN_CELSHADE, "#define APPLY_TC_GEN_CELSHADE\n", "_tc_cel" },
	{ GLSL_SHADER_Q3_TC_GEN_PROJECTION, "#define APPLY_TC_GEN_PROJECTION\n", "_tc_proj" },
	{ GLSL_SHADER_Q3_TC_GEN_REFLECTION, "#define APPLY_TC_GEN_REFLECTION\n", "_tc_refl" },
	{ GLSL_SHADER_Q3_TC_GEN_ENV, "#define APPLY_TC_GEN_ENV\n", "_tc_env" },
	{ GLSL_SHADER_Q3_TC_GEN_VECTOR, "#define APPLY_TC_GEN_VECTOR\n", "_tc_vec" },
	{ GLSL_SHADER_Q3_TC_GEN_SURROUND, "#define APPLY_TC_GEN_SURROUND\n", "_tc_surr" },

	{ GLSL_SHADER_Q3_LIGHTSTYLE3, "#define NUM_LIGHTMAPS 4\n#define qf_lmvec01 vec4\n#define qf_lmvec23 vec4\n", "_ls3" },
	{ GLSL_SHADER_Q3_LIGHTSTYLE2, "#define NUM_LIGHTMAPS 3\n#define qf_lmvec01 vec4\n#define qf_lmvec23 vec2\n", "_ls2" },
	{ GLSL_SHADER_Q3_LIGHTSTYLE1, "#define NUM_LIGHTMAPS 2\n#define qf_lmvec01 vec4\n", "_ls1" },
	{ GLSL_SHADER_Q3_LIGHTSTYLE0, "#define NUM_LIGHTMAPS 1\n#define qf_lmvec01 vec2\n", "_ls0" },

	{ GLSL_SHADER_Q3_LIGHTMAP_ARRAYS, "#define LIGHTMAP_ARRAYS\n", "_lmarray" },

	{ GLSL_SHADER_Q3_ALPHA_MASK, "#define APPLY_ALPHA_MASK\n", "_alpha_mask" },

	{ 0, NULL, NULL } };

static const glsl_feature_t glsl_features_celshade[] = {
	{ GLSL_SHADER_COMMON_GREYSCALE, "#define APPLY_GREYSCALE\n", "_grey" },

	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS4, "#define QF_NUM_BONE_INFLUENCES 4\n", "_bones4" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS3, "#define QF_NUM_BONE_INFLUENCES 3\n", "_bones3" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS2, "#define QF_NUM_BONE_INFLUENCES 2\n", "_bones2" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS1, "#define QF_NUM_BONE_INFLUENCES 1\n", "_bones1" },

	{ GLSL_SHADER_COMMON_AUTOSPRITE, "#define APPLY_AUTOSPRITE\n", "" },
	{ GLSL_SHADER_COMMON_AUTOSPRITE2, "#define APPLY_AUTOSPRITE2\n", "" },
	{ GLSL_SHADER_COMMON_AUTOPARTICLE, "#define APPLY_AUTOSPRITE\n#define APPLY_AUTOPARTICLE\n", "" },

	{ GLSL_SHADER_COMMON_RGB_GEN_ONE_MINUS_VERTEX, "#define APPLY_RGB_ONE_MINUS_VERTEX\n", "_c1-v" },
	{ GLSL_SHADER_COMMON_RGB_GEN_CONST, "#define APPLY_RGB_CONST\n", "_cc" },
	{ GLSL_SHADER_COMMON_RGB_GEN_VERTEX, "#define APPLY_RGB_VERTEX\n", "_cv" },
	{ GLSL_SHADER_COMMON_RGB_GEN_DIFFUSELIGHT, "#define APPLY_RGB_GEN_DIFFUSELIGHT\n", "_cl" },

	{ GLSL_SHADER_COMMON_ALPHA_GEN_ONE_MINUS_VERTEX, "#define APPLY_ALPHA_ONE_MINUS_VERTEX\n", "_a1-v" },
	{ GLSL_SHADER_COMMON_ALPHA_GEN_VERTEX, "#define APPLY_ALPHA_VERTEX\n", "_av" },
	{ GLSL_SHADER_COMMON_ALPHA_GEN_CONST, "#define APPLY_ALPHA_CONST\n", "_ac" },

	{ GLSL_SHADER_COMMON_FOG, "#define APPLY_FOG\n#define APPLY_FOG_IN 1\n", "_fog" },
	{ GLSL_SHADER_COMMON_FOG_RGB, "#define APPLY_FOG_COLOR\n", "_rgb" },

	{ GLSL_SHADER_COMMON_INSTANCED_TRANSFORMS, "#define APPLY_INSTANCED_TRANSFORMS\n", "_instanced" },
	{ GLSL_SHADER_COMMON_INSTANCED_ATTRIB_TRANSFORMS, "#define APPLY_INSTANCED_TRANSFORMS\n#define APPLY_INSTANCED_ATTRIB_TRANSFORMS\n", "_instanced_va" },

	{ GLSL_SHADER_COMMON_AFUNC_GE128, "#define QF_ALPHATEST(a) { if ((a) < 0.5) discard; }\n", "_afunc_ge128" },
	{ GLSL_SHADER_COMMON_AFUNC_LT128, "#define QF_ALPHATEST(a) { if ((a) >= 0.5) discard; }\n", "_afunc_lt128" },
	{ GLSL_SHADER_COMMON_AFUNC_GT0, "#define QF_ALPHATEST(a) { if ((a) <= 0.0) discard; }\n", "_afunc_gt0" },

	{ GLSL_SHADER_COMMON_TC_MOD, "#define APPLY_TC_MOD\n", "_tc_mod" },

	{ GLSL_SHADER_CELSHADE_DIFFUSE, "#define APPLY_DIFFUSE\n", "_diff" },
	{ GLSL_SHADER_CELSHADE_DECAL, "#define APPLY_DECAL\n", "_decal" },
	{ GLSL_SHADER_CELSHADE_DECAL_ADD, "#define APPLY_DECAL_ADD\n", "_decal" },
	{ GLSL_SHADER_CELSHADE_ENTITY_DECAL, "#define APPLY_ENTITY_DECAL\n", "_edecal" },
	{ GLSL_SHADER_CELSHADE_ENTITY_DECAL_ADD, "#define APPLY_ENTITY_DECAL_ADD\n", "_add" },
	{ GLSL_SHADER_CELSHADE_STRIPES, "#define APPLY_STRIPES\n", "_stripes" },
	{ GLSL_SHADER_CELSHADE_STRIPES_ADD, "#define APPLY_STRIPES_ADD\n", "_stripes_add" },
	{ GLSL_SHADER_CELSHADE_CEL_LIGHT, "#define APPLY_CEL_LIGHT\n", "_light" },
	{ GLSL_SHADER_CELSHADE_CEL_LIGHT_ADD, "#define APPLY_CEL_LIGHT_ADD\n", "_add" },

	{ 0, NULL, NULL } };

static const glsl_feature_t glsl_features_fog[] = {
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS4, "#define QF_NUM_BONE_INFLUENCES 4\n", "_bones4" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS3, "#define QF_NUM_BONE_INFLUENCES 3\n", "_bones3" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS2, "#define QF_NUM_BONE_INFLUENCES 2\n", "_bones2" },
	{ GLSL_SHADER_COMMON_BONE_TRANSFORMS1, "#define QF_NUM_BONE_INFLUENCES 1\n", "_bones1" },

	{ GLSL_SHADER_COMMON_FOG, "#define APPLY_FOG\n", "_fog" },
	{ GLSL_SHADER_COMMON_FOG_RGB, "#define APPLY_FOG_COLOR\n", "_rgb" },

	{ GLSL_SHADER_COMMON_AUTOSPRITE, "#define APPLY_AUTOSPRITE\n", "" },
	{ GLSL_SHADER_COMMON_AUTOSPRITE2, "#define APPLY_AUTOSPRITE2\n", "" },
	{ GLSL_SHADER_COMMON_AUTOPARTICLE, "#define APPLY_AUTOSPRITE\n#define APPLY_AUTOPARTICLE\n", "" },

	{ GLSL_SHADER_COMMON_INSTANCED_TRANSFORMS, "#define APPLY_INSTANCED_TRANSFORMS\n", "_instanced" },
	{ GLSL_SHADER_COMMON_INSTANCED_ATTRIB_TRANSFORMS, "#define APPLY_INSTANCED_TRANSFORMS\n#define APPLY_INSTANCED_ATTRIB_TRANSFORMS\n", "_instanced_va" },

	{ 0, NULL, NULL } };

static const glsl_feature_t glsl_features_fxaa[] = { { GLSL_SHADER_FXAA_FXAA3, "#define APPLY_FXAA3\n", "_fxaa3" },

													 { 0, NULL, NULL } };

static const glsl_feature_t *const glsl_programtypes_features[] = {
	// GLSL_PROGRAM_TYPE_NONE
	NULL,
	[GLSL_PROGRAM_TYPE_MATERIAL] = glsl_features_material,
	[GLSL_PROGRAM_TYPE_DISTORTION] = glsl_features_distortion,
	[GLSL_PROGRAM_TYPE_RGB_SHADOW] = glsl_features_rgbshadow,
	[GLSL_PROGRAM_TYPE_SHADOWMAP] = glsl_features_shadowmap,
	[GLSL_PROGRAM_TYPE_OUTLINE] = glsl_features_outline,
	[GLSL_PROGRAM_TYPE_UNUSED] = glsl_features_empty,
	[GLSL_PROGRAM_TYPE_Q3A_SHADER] = glsl_features_q3a,
	[GLSL_PROGRAM_TYPE_CELSHADE] = glsl_features_celshade,
	[GLSL_PROGRAM_TYPE_FOG] = glsl_features_fog,
	[GLSL_PROGRAM_TYPE_FXAA] = glsl_features_fxaa,
	[GLSL_PROGRAM_TYPE_YUV] = glsl_features_empty,
	[GLSL_PROGRAM_TYPE_COLORCORRECTION] = glsl_features_empty };

#define QF_GLSL_VERSION120 "#version 120\n"
#define QF_GLSL_VERSION130 "#version 130\n"
#define QF_GLSL_VERSION140 "#version 140\n"
#define QF_GLSL_VERSION300ES "#version 300 es\n"
#define QF_GLSL_VERSION310ES "#version 310 es\n"

#define QF_GLSL_ENABLE_ARB_GPU_SHADER5 "#extension GL_ARB_gpu_shader5 : enable\n"
#define QF_GLSL_ENABLE_EXT_GPU_SHADER5 "#extension GL_EXT_gpu_shader5 : enable\n"
#define QF_GLSL_ENABLE_ARB_DRAW_INSTANCED "#extension GL_ARB_draw_instanced : enable\n"
#define QF_GLSL_ENABLE_EXT_SHADOW_SAMPLERS "#extension GL_EXT_shadow_samplers : enable\n"
#define QF_GLSL_ENABLE_EXT_TEXTURE_ARRAY "#extension GL_EXT_texture_array : enable\n"
#define QF_GLSL_ENABLE_OES_TEXTURE_3D "#extension GL_OES_texture_3D : enable\n"

#define QF_GLSL_PI                             \
	""                                         \
	"#ifndef M_PI\n"                           \
	"#define M_PI 3.14159265358979323846\n"    \
	"#endif\n"                                 \
	"#ifndef M_TWOPI\n"                        \
	"#define M_TWOPI 6.28318530717958647692\n" \
	"#endif\n"

#define QF_BUILTIN_GLSL_CONSTANTS \
	QF_GLSL_PI                    \
	"\n" \
"#define MAX_UNIFORM_INSTANCES " STR_TOSTR( MAX_GLSL_UNIFORM_INSTANCES ) "\n" \
"#define MAX_UNIFORM_BONES " STR_TOSTR( MAX_GLSL_UNIFORM_INSTANCES ) "\n"

#define QF_BUILTIN_GLSL_QUAT_TRANSFORM_OVERLOAD                                                                       \
	"#ifdef QF_DUAL_QUAT_TRANSFORM_TANGENT\n"                                                                         \
	"void QF_VertexDualQuatsTransform_Tangent(inout vec4 Position, inout vec3 Normal, inout vec3 Tangent)\n"          \
	"#else\n"                                                                                                         \
	"void QF_VertexDualQuatsTransform(inout vec4 Position, inout vec3 Normal)\n"                                      \
	"#endif\n"                                                                                                        \
	"{\n"                                                                                                             \
	"	ivec4 Indices = ivec4(a_BonesIndices * 2.0);\n"                                                                 \
	"	vec4 DQReal = u_DualQuats[Indices.x];\n"                                                                        \
	"	vec4 DQDual = u_DualQuats[Indices.x + 1];\n"                                                                    \
	"#if QF_NUM_BONE_INFLUENCES >= 2\n"                                                                               \
	"	DQReal *= a_BonesWeights.x;\n"                                                                                  \
	"	DQDual *= a_BonesWeights.x;\n"                                                                                  \
	"	vec4 DQReal1 = u_DualQuats[Indices.y];\n"                                                                       \
	"	vec4 DQDual1 = u_DualQuats[Indices.y + 1];\n"                                                                   \
	"	float Scale = mix(-1.0, 1.0, step(0.0, dot(DQReal1, DQReal))) * a_BonesWeights.y;\n"                            \
	"	DQReal += DQReal1 * Scale;\n"                                                                                   \
	"	DQDual += DQDual1 * Scale;\n"                                                                                   \
	"#if QF_NUM_BONE_INFLUENCES >= 3\n"                                                                               \
	"	DQReal1 = u_DualQuats[Indices.z];\n"                                                                            \
	"	DQDual1 = u_DualQuats[Indices.z + 1];\n"                                                                        \
	"	Scale = mix(-1.0, 1.0, step(0.0, dot(DQReal1, DQReal))) * a_BonesWeights.z;\n"                                  \
	"	DQReal += DQReal1 * Scale;\n"                                                                                   \
	"	DQDual += DQDual1 * Scale;\n"                                                                                   \
	"#if QF_NUM_BONE_INFLUENCES >= 4\n"                                                                               \
	"	DQReal1 = u_DualQuats[Indices.w];\n"                                                                            \
	"	DQDual1 = u_DualQuats[Indices.w + 1];\n"                                                                        \
	"	Scale = mix(-1.0, 1.0, step(0.0, dot(DQReal1, DQReal))) * a_BonesWeights.w;\n"                                  \
	"	DQReal += DQReal1 * Scale;\n"                                                                                   \
	"	DQDual += DQDual1 * Scale;\n"                                                                                   \
	"#endif // QF_NUM_BONE_INFLUENCES >= 4\n"                                                                         \
	"#endif // QF_NUM_BONE_INFLUENCES >= 3\n"                                                                         \
	"	float Len = 1.0 / length(DQReal);\n"                                                                            \
	"	DQReal *= Len;\n"                                                                                               \
	"	DQDual *= Len;\n"                                                                                               \
	"#endif // QF_NUM_BONE_INFLUENCES >= 2\n"                                                                         \
	"	Position.xyz += (cross(DQReal.xyz, cross(DQReal.xyz, Position.xyz) + Position.xyz * DQReal.w + DQDual.xyz) +\n" \
	"		DQDual.xyz*DQReal.w - DQReal.xyz*DQDual.w) * 2.0;\n"                                                           \
	"	Normal += cross(DQReal.xyz, cross(DQReal.xyz, Normal) + Normal * DQReal.w) * 2.0;\n"                            \
	"#ifdef QF_DUAL_QUAT_TRANSFORM_TANGENT\n"                                                                         \
	"	Tangent += cross(DQReal.xyz, cross(DQReal.xyz, Tangent) + Tangent * DQReal.w) * 2.0;\n"                         \
	"#endif\n"                                                                                                        \
	"}\n"                                                                                                             \
	"\n"

#define QF_BUILTIN_GLSL_QUAT_TRANSFORM                    \
	"qf_attribute vec4 a_BonesIndices, a_BonesWeights;\n" \
	"uniform vec4 u_DualQuats[MAX_UNIFORM_BONES*2];\n"    \
	"\n" QF_BUILTIN_GLSL_QUAT_TRANSFORM_OVERLOAD "#define QF_DUAL_QUAT_TRANSFORM_TANGENT\n" QF_BUILTIN_GLSL_QUAT_TRANSFORM_OVERLOAD "#undef QF_DUAL_QUAT_TRANSFORM_TANGENT\n"

#define QF_BUILTIN_GLSL_INSTANCED_TRANSFORMS                                                                           \
	"#if defined(APPLY_INSTANCED_ATTRIB_TRANSFORMS)\n"                                                                 \
	"qf_attribute vec4 a_InstanceQuat, a_InstancePosAndScale;\n"                                                       \
	"#elif defined(GL_ARB_draw_instanced) || (defined(GL_ES) && (__VERSION__ >= 300))\n"                               \
	"uniform vec4 u_InstancePoints[MAX_UNIFORM_INSTANCES*2];\n"                                                        \
	"#define a_InstanceQuat u_InstancePoints[gl_InstanceID*2]\n"                                                       \
	"#define a_InstancePosAndScale u_InstancePoints[gl_InstanceID*2+1]\n"                                              \
	"#else\n"                                                                                                          \
	"uniform vec4 u_InstancePoints[2];\n"                                                                              \
	"#define a_InstanceQuat u_InstancePoints[0]\n"                                                                     \
	"#define a_InstancePosAndScale u_InstancePoints[1]\n"                                                              \
	"#endif // APPLY_INSTANCED_ATTRIB_TRANSFORMS\n"                                                                    \
	"\n"                                                                                                               \
	"void QF_InstancedTransform(inout vec4 Position, inout vec3 Normal)\n"                                             \
	"{\n"                                                                                                              \
	"	Position.xyz = (cross(a_InstanceQuat.xyz,\n"                                                                     \
	"		cross(a_InstanceQuat.xyz, Position.xyz) + Position.xyz*a_InstanceQuat.w)*2.0 +\n"                               \
	"		Position.xyz) * a_InstancePosAndScale.w + a_InstancePosAndScale.xyz;\n"                                         \
	"	Normal = cross(a_InstanceQuat.xyz, cross(a_InstanceQuat.xyz, Normal) + Normal*a_InstanceQuat.w)*2.0 + Normal;\n" \
	"}\n"                                                                                                              \
	"\n"

// We have to use these #ifdefs here because #defining prototypes
// of these functions to nothing results in a crash on Intel GPUs.
#define QF_BUILTIN_GLSL_TRANSFORM_VERTS                                                                                 \
	"void QF_TransformVerts(inout vec4 Position, inout vec3 Normal, inout vec2 TexCoord)\n"                             \
	"{\n"                                                                                                               \
	"#	ifdef QF_NUM_BONE_INFLUENCES\n"                                                                                  \
	"		QF_VertexDualQuatsTransform(Position, Normal);\n"                                                                \
	"#	endif\n"                                                                                                         \
	"#	ifdef QF_APPLY_DEFORMVERTS\n"                                                                                    \
	"		QF_DeformVerts(Position, Normal, TexCoord);\n"                                                                   \
	"#	endif\n"                                                                                                         \
	"#	ifdef APPLY_INSTANCED_TRANSFORMS\n"                                                                              \
	"		QF_InstancedTransform(Position, Normal);\n"                                                                      \
	"#	endif\n"                                                                                                         \
	"}\n"                                                                                                               \
	"\n"                                                                                                                \
	"void QF_TransformVerts_Tangent(inout vec4 Position, inout vec3 Normal, inout vec3 Tangent, inout vec2 TexCoord)\n" \
	"{\n"                                                                                                               \
	"#	ifdef QF_NUM_BONE_INFLUENCES\n"                                                                                  \
	"		QF_VertexDualQuatsTransform_Tangent(Position, Normal, Tangent);\n"                                               \
	"#	endif\n"                                                                                                         \
	"#	ifdef QF_APPLY_DEFORMVERTS\n"                                                                                    \
	"		QF_DeformVerts(Position, Normal, TexCoord);\n"                                                                   \
	"#	endif\n"                                                                                                         \
	"#	ifdef APPLY_INSTANCED_TRANSFORMS\n"                                                                              \
	"		QF_InstancedTransform(Position, Normal);\n"                                                                      \
	"#	endif\n"                                                                                                         \
	"}\n"                                                                                                               \
	"\n"

#define QF_GLSL_WAVEFUNCS                                                                                                                      \
	"\n" QF_GLSL_PI                                                                                                                            \
	"\n"                                                                                                                                       \
	"#ifndef WAVE_SIN\n"                                                                                                                       \
	"float QF_WaveFunc_Sin(float x)\n"                                                                                                         \
	"{\n"                                                                                                                                      \
	"return sin(fract(x) * M_TWOPI);\n"                                                                                                        \
	"}\n"                                                                                                                                      \
	"float QF_WaveFunc_Triangle(float x)\n"                                                                                                    \
	"{\n"                                                                                                                                      \
	"x = fract(x);\n"                                                                                                                          \
	"return step(x, 0.25) * x * 4.0 + (2.0 - 4.0 * step(0.25, x) * step(x, 0.75) * x) + ((step(0.75, x) * x - 0.75) * 4.0 - 1.0);\n"           \
	"}\n"                                                                                                                                      \
	"float QF_WaveFunc_Square(float x)\n"                                                                                                      \
	"{\n"                                                                                                                                      \
	"return step(fract(x), 0.5) * 2.0 - 1.0;\n"                                                                                                \
	"}\n"                                                                                                                                      \
	"float QF_WaveFunc_Sawtooth(float x)\n"                                                                                                    \
	"{\n"                                                                                                                                      \
	"return fract(x);\n"                                                                                                                       \
	"}\n"                                                                                                                                      \
	"float QF_WaveFunc_InverseSawtooth(float x)\n"                                                                                             \
	"{\n"                                                                                                                                      \
	"return 1.0 - fract(x);\n"                                                                                                                 \
	"}\n"                                                                                                                                      \
	"\n"                                                                                                                                       \
	"#define WAVE_SIN(time,base,amplitude,phase,freq) (((base)+(amplitude)*QF_WaveFunc_Sin((phase)+(time)*(freq))))\n"                         \
	"#define WAVE_TRIANGLE(time,base,amplitude,phase,freq) (((base)+(amplitude)*QF_WaveFunc_Triangle((phase)+(time)*(freq))))\n"               \
	"#define WAVE_SQUARE(time,base,amplitude,phase,freq) (((base)+(amplitude)*QF_WaveFunc_Square((phase)+(time)*(freq))))\n"                   \
	"#define WAVE_SAWTOOTH(time,base,amplitude,phase,freq) (((base)+(amplitude)*QF_WaveFunc_Sawtooth((phase)+(time)*(freq))))\n"               \
	"#define WAVE_INVERSESAWTOOTH(time,base,amplitude,phase,freq) (((base)+(amplitude)*QF_WaveFunc_InverseSawtooth((phase)+(time)*(freq))))\n" \
	"#endif\n"                                                                                                                                 \
	"\n"

#define QF_GLSL_MATH                                                                                        \
	"#define QF_LatLong2Norm(ll) vec3(cos((ll).y) * sin((ll).x), sin((ll).y) * sin((ll).x), cos((ll).x))\n" \
	"\n"

#define PARSER_MAX_STACKDEPTH 16

static bool __RF_AppendShaderFromFile( struct QStr *str, const char *rootFile, const char *fileName, int stackDepth, int programType, r_glslfeat_t features )
{
	char tempbuf[MAX_TOKEN_CHARS];
	char *fileContents = NULL;
	char *trieCache = NULL;
	trie_error_t trie_error = Trie_Find( glsl_cache_trie, fileName, TRIE_EXACT_MATCH, (void **)&trieCache );
	if( trie_error != TRIE_OK ) {
		R_LoadFile( fileName, (void **)&fileContents );
		if( fileContents ) {
			trieCache = R_CopyString( fileContents );
		}
		Trie_Insert( glsl_cache_trie, fileName, trieCache );
	} else {
		if( trieCache ) {
			fileContents = R_CopyString( trieCache );
		}
	}

	if( !fileContents ) {
		Com_Printf( S_COLOR_YELLOW "Cannot load file '%s'\n", fileName );
		return true;
	}

	char *it = fileContents;
	char *prevIt = NULL;
	char *startBuf = NULL;
	bool error = false;
	struct QStr includeFilePath = { 0 };
	while( 1 ) {
		prevIt = it;
		char *token = COM_ParseExt_r( tempbuf, sizeof( tempbuf ), &it, true );
		if( !token[0] ) {
			break;
		}
		bool include = false;
		bool ignore_include = false;

		if( !Q_stricmp( token, "#include" ) ) {
			include = true;
		} else if( !Q_strnicmp( token, "#include_if(", 12 ) ) {
			include = true;
			token += 12;

			ignore_include = true;
			if( ( !Q_stricmp( token, "APPLY_FOG)" ) && ( features & GLSL_SHADER_COMMON_FOG ) ) ||

				( !Q_stricmp( token, "NUM_DLIGHTS)" ) && ( features & GLSL_SHADER_COMMON_DLIGHTS ) ) ||

				( !Q_stricmp( token, "APPLY_GREYSCALE)" ) && ( features & GLSL_SHADER_COMMON_GREYSCALE ) ) ||

				( ( programType == GLSL_PROGRAM_TYPE_Q3A_SHADER ) && !Q_stricmp( token, "NUM_LIGHTMAPS)" ) && ( features & GLSL_SHADER_Q3_LIGHTSTYLE ) ) ||

				( ( programType == GLSL_PROGRAM_TYPE_MATERIAL ) && !Q_stricmp( token, "NUM_LIGHTMAPS)" ) && ( features & GLSL_SHADER_MATERIAL_LIGHTSTYLE ) ) ||

				( ( programType == GLSL_PROGRAM_TYPE_MATERIAL ) && !Q_stricmp( token, "APPLY_OFFSETMAPPING)" ) &&
				  ( features & ( GLSL_SHADER_MATERIAL_OFFSETMAPPING | GLSL_SHADER_MATERIAL_RELIEFMAPPING ) ) ) ||

				( ( programType == GLSL_PROGRAM_TYPE_MATERIAL ) && !Q_stricmp( token, "APPLY_CELSHADING)" ) && ( features & GLSL_SHADER_MATERIAL_CELSHADING ) ) ||

				( ( programType == GLSL_PROGRAM_TYPE_MATERIAL ) && !Q_stricmp( token, "APPLY_DIRECTIONAL_LIGHT)" ) && ( features & GLSL_SHADER_MATERIAL_DIRECTIONAL_LIGHT ) )

			) {
				ignore_include = false;
			}
		}

		char *line = token;
		if( !include || ignore_include ) {
			if( !ignore_include ) {
				if( !startBuf ) {
					startBuf = prevIt;
				}
			}

			// skip to the end of the line
			token = strchr( it, '\n' );
			if( !token ) {
				break;
			}
			it = token + 1;
			continue;
		}

		if( startBuf && prevIt > startBuf ) {
			// cut the string at the beginning of the #include
			*prevIt = '\0';
			qstrcatfmt( str, "%s", startBuf );
			startBuf = NULL;
		}

		// parse #include argument
		token = COM_Parse_r( tempbuf, sizeof( tempbuf ), &it );
		if( !token[0] ) {
			Com_Printf( S_COLOR_YELLOW "Syntax error in '%s' around '%s'\n", fileName, line );
			error = true;
			goto done;
		}

		if( stackDepth == PARSER_MAX_STACKDEPTH ) {
			Com_Printf( S_COLOR_YELLOW "Include stack overflow in '%s' around '%s'\n", fileName, line );
			error = true;
			goto done;
		}

		// load files from current directory, unless the path starts
		// with the leading "/". in that case, go back to to top directory
		COM_SanitizeFilePath( token );

		qStrClear( &includeFilePath );
		qStrMakeRoomFor( &includeFilePath, strlen( fileName ) + 1 + strlen( token ) + 1 );

		// tempFilename = R_Malloc( tempFilenameSize );

		if( *token != '/' ) {
			qStrAppendSlice( &includeFilePath, qCToStrRef( fileName ) );
			COM_StripFilename( includeFilePath.buf );
		} else {
			token++;
			qStrAppendSlice( &includeFilePath, qCToStrRef( rootFile ) );
			COM_StripFilename( includeFilePath.buf );
		}
		qStrUpdateLen( &includeFilePath );
		qStrSetNullTerm( &includeFilePath );
		qStrAppendSlice( &includeFilePath, ( includeFilePath.buf[0] ? qCToStrRef( "/" ) : qCToStrRef( "" ) ) );
		qStrAppendSlice( &includeFilePath, qCToStrRef( token ) );
		if( __RF_AppendShaderFromFile( str, rootFile, includeFilePath.buf, stackDepth + 1, programType, features ) ) {
			error = true;
			goto done;
		}
	}
	if( startBuf ) {
		qstrcatfmt( str, "%s", startBuf );
	}
done:
	R_Free( fileContents );
	qStrFree( &includeFilePath );
	return error;
}

static bool RF_AppendShaderFromFile( struct QStr *str, const char *fileName, int programType, r_glslfeat_t features )
{
	return __RF_AppendShaderFromFile( str, fileName, fileName, 1, programType, features );
}

void RP_ProgramList_f( void )
{
	Com_Printf( "------------------\n" );
	size_t i;
	struct glsl_program_s *program;
	struct QStr fullName = { 0 };
	for( i = 0, program = r_glslprograms; i < MAX_GLSL_PROGRAMS; i++, program++ ) {
		if( !program->name )
			break;

		qStrClear( &fullName );
		qStrAppendSlice( &fullName, qCToStrRef( program->name ) );
		for( feature_iter_t iter = __R_NextFeature( (feature_iter_t){ .it = glsl_programtypes_features[program->type], .bits = program->features } ); __R_IsValidFeatureIter( &iter );
			 iter = __R_NextFeature( iter ) ) {
			qStrAppendSlice( &fullName, qCToStrRef( iter.it->suffix ) );
		}
		Com_Printf( " %3i %.*s", i + 1, fullName.len, fullName.buf );

		if( *program->deformsKey ) {
			Com_Printf( " dv:%s", program->deformsKey );
		}
		Com_Printf( "\n" );
	}
	qStrFree( &fullName );
	Com_Printf( "%i programs total\n", i );
}

const char *RP_GLSLStageToShaderPrefix( const glsl_program_stage_t stage )
{
	switch( stage ) {
		case GLSL_STAGE_VERTEX:
			return "vert";
		case GLSL_STAGE_FRAGMENT:
			return "frag";
		default:
			assert( 0 ); // unhandled
			break;
	}
	return "";
}

static void __RP_writeTextToFile( const char *filePath, const char *str )
{
	int shaderErrHandle = 0;
	if( FS_FOpenFile( filePath, &shaderErrHandle, FS_WRITE ) == -1 ) {
		Com_Printf( S_COLOR_YELLOW "Could not open %s for writing.\n", filePath );
	} else {
		FS_Write( str, strlen( str ), shaderErrHandle );
		FS_FCloseFile( shaderErrHandle );
	}
}

static inline struct pipeline_hash_s *__resolvePipeline( struct glsl_program_s *program, hash_t hash )
{
	const size_t startIndex = hash % PIPELINE_LAYOUT_HASH_SIZE;
	size_t index = startIndex;
	do {
		if( program->pipelines[index].hash == hash || program->pipelines[index].hash == 0 ) {
			program->pipelines[index].hash = hash;
			return &program->pipelines[index];
		}
		index = ( index + 1 ) % PIPELINE_LAYOUT_HASH_SIZE;

	} while( index != startIndex );
	return NULL;
}

#if ( DEVICE_IMPL_VULKAN )
static int __VK_SortVkVertexInputAttributeDescription( const void *a1, const void *a2 )
{
	const struct VkVertexInputAttributeDescription *at1 = a1;
	const struct VkVertexInputAttributeDescription *at2 = a2;
	return at1->location > at2->location;
}
#endif

#if ( DEVICE_IMPL_MTL )
// Metal keeps the depth test + write flag in an MTLDepthStencilState object rather than in the
// pipeline, and the state space is tiny -- one compare function by write-on/write-off -- so every
// combination is built once on first use and kept for the life of the process. Building one per draw
// would be a per-draw object allocation on the hot path.
	#define R_MTL_COMPARE_FUNC_COUNT ( RI_COMPARE_GREATER_EQUAL + 1 )
static struct mtlc_depth_stencil_state s_mtlDepthStates[R_MTL_COMPARE_FUNC_COUNT][2];
static bool s_mtlDepthStateBuilt[R_MTL_COMPARE_FUNC_COUNT][2];

static struct mtlc_depth_stencil_state __RP_MTLResolveDepthState( enum RICompareFunc_e func, bool depthWrite )
{
	assert( (unsigned)func < R_MTL_COMPARE_FUNC_COUNT );
	const unsigned write = depthWrite ? 1 : 0;
	if( !s_mtlDepthStateBuilt[func][write] ) {
		struct mtlc_depth_stencil_descriptor dsd = mtlc_depth_stencil_descriptor_init();
		mtlc_depth_stencil_descriptor_set_depth_compare_function( dsd, RICompareOpToMTL( func ) );
		mtlc_depth_stencil_descriptor_set_depth_write_enabled( dsd, depthWrite );
		s_mtlDepthStates[func][write] = mtlc_device_new_depth_stencil_state( rsh.device.mtl.device, dsd );
		mtlc_depth_stencil_descriptor_release( dsd );
		s_mtlDepthStateBuilt[func][write] = true;
	}
	return s_mtlDepthStates[func][write];
}

static void __RP_MTLReleaseDepthStates( void )
{
	for( unsigned func = 0; func < R_MTL_COMPARE_FUNC_COUNT; func++ ) {
		for( unsigned write = 0; write < 2; write++ ) {
			if( !s_mtlDepthStateBuilt[func][write] )
				continue;
			mtlc_depth_stencil_state_release( s_mtlDepthStates[func][write] );
			s_mtlDepthStateBuilt[func][write] = false;
		}
	}
}
#endif

void RP_BindPipeline( struct FrameState_s *cmd, struct pipeline_hash_s *pipeline )
{
	TracyCZoneN( ctx, "RP_BindPipeline", 1 );
#if ( DEVICE_IMPL_VULKAN )
	vkCmdBindPipeline( cmd->handle.vk.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vk.handle );
#endif
#if ( DEVICE_IMPL_MTL )
	if( !mtlc_render_command_encoder_is_nil( cmd->handle.mtl.encoder ) ) {
		const struct mtlc_render_command_encoder enc = cmd->handle.mtl.encoder;
		const struct pipeline_desc_s *desc = &cmd->pipeline;

		// A nil state means the program failed to build; skip the bind rather than assert so one bad
		// shader does not take the frame down.
		if( pipeline->mtl.state )
			mtlc_render_command_encoder_set_render_pipeline_state( enc, mtlc_render_pipeline_state_from_id( pipeline->mtl.state ) );

		// Everything below is baked into VkGraphicsPipelineCreateInfo on Vulkan (see RP_ResolvePipeline)
		// but is encoder state on Metal, which is why it is applied per bind and is deliberately not part
		// of the Metal pipeline cache key. Encoder state does not survive a new encoder, so this must run
		// on every bind rather than only when the pipeline object changes.

		// Vulkan disables depth writes whenever depthTestEnable is false, and Metal raises a validation
		// error for a depth write with no depth attachment. RICompareOpToMTL maps NONE -> ALWAYS (Metal
		// has no separate enable), so both conditions have to gate the write flag here instead.
		const bool hasDepth = ( desc->depthFormat != RI_FORMAT_UNKNOWN );
		const bool depthWrite = desc->depthWrite && hasDepth && ( desc->compareFunc != RI_COMPARE_NONE );
		mtlc_render_command_encoder_set_depth_stencil_state( enc, __RP_MTLResolveDepthState( (enum RICompareFunc_e)desc->compareFunc, depthWrite ) );

		mtlc_render_command_encoder_set_cull_mode( enc, RICullModeToMTL( FR_PipelineCullMode( desc ) ) );
		// Matches the VK arm's VK_FRONT_FACE_COUNTER_CLOCKWISE; Metal defaults to clockwise.
		mtlc_render_command_encoder_set_front_facing_winding( enc, MTLC_WINDING_COUNTER_CLOCKWISE );

		// Same enable predicate the VK arm uses -- Metal has no depthBiasEnable, so "off" is all zeroes.
		const bool useDepthBias = ( desc->depthBiasConstant != 0 || desc->depthBiasSlope != 0 );
		mtlc_render_command_encoder_set_depth_bias( enc, useDepthBias ? desc->depthBiasConstant : 0.0f,
													useDepthBias ? desc->depthBiasSlope : 0.0f,
													useDepthBias ? desc->depthBiasClamp : 0.0f );
	}
#endif
	TracyCZoneEnd( ctx );
}

struct pipeline_hash_s *RP_ResolvePipeline( struct glsl_program_s *program, struct pipeline_desc_s *cmd )
{
	const enum RICullMode_e cullMode = FR_PipelineCullMode( cmd );
	struct pipeline_hash_s *pipeline = NULL;
#if ( DEVICE_IMPL_VULKAN )
	VkVertexInputAttributeDescription vertextbindingDesc[MAX_ATTRIBUTES];
	VkVertexInputBindingDescription vertexInputStreamsDesc[MAX_STREAMS];
	VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VkFormat colorAttachmentsVK[MAX_COLOR_ATTACHMENTS];

	const bool useDepthBias = ( cmd->depthBiasConstant != 0 || cmd->depthBiasSlope != 0 );

	//	uint32_t attribFlags = 0;
	assert( cmd->numAttribs <= MAX_ATTRIBUTES );
	assert( cmd->numStreams <= MAX_STREAMS );
	assert( cmd->numColorsAttachments <= MAX_COLOR_ATTACHMENTS );
	if( cmd->numStreams > 0 ) {
		uint32_t numVertexAttribs = 0;
		for( size_t i = 0; i < cmd->numAttribs; i++ ) {
			if( !( ( 1 << cmd->attribs[i].vk.location ) & program->vertexInputMask ) ) {
				continue;
			}
			vertextbindingDesc[numVertexAttribs].offset = cmd->attribs[i].offset;
			vertextbindingDesc[numVertexAttribs].binding = cmd->attribs[i].streamIndex;
			vertextbindingDesc[numVertexAttribs].format = RIFormatToVK( cmd->attribs[i].format );
			vertextbindingDesc[numVertexAttribs].location = cmd->attribs[i].vk.location;
			numVertexAttribs++;
		}

		for( size_t i = 0; i < cmd->numStreams; i++ ) {
			vertexInputStreamsDesc[i].binding = cmd->streams[i].bindingSlot;
			vertexInputStreamsDesc[i].stride = cmd->streams[i].stride;
			vertexInputStreamsDesc[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		}
		vertexInputState.pVertexAttributeDescriptions = vertextbindingDesc;
		vertexInputState.vertexAttributeDescriptionCount = numVertexAttribs;
		vertexInputState.pVertexBindingDescriptions = vertexInputStreamsDesc;
		vertexInputState.vertexBindingDescriptionCount = cmd->numStreams;
	}
	qsort( vertextbindingDesc, vertexInputState.vertexAttributeDescriptionCount, sizeof( VkVertexInputAttributeDescription ), __VK_SortVkVertexInputAttributeDescription );

	struct {
		float depthBiasConstant;
		float depthBiasClamp;
		float depthBiasSlope;
		uint16_t cullMode : 3; // RICullMode_e
		uint16_t colorBlendEnabled : 1;
		uint16_t depthWrite : 1;
		uint16_t colorWriteMask : 4; // RIColorWriteMask_e
		uint16_t colorSrcFactor : 5; // RIBlendFactor_e
		uint16_t colorDstFactor : 5; // RIBlendFactor_e
		uint16_t topology : 4;		 // RITopology_e
		uint16_t compareFunc : 4;	 // RICompareFunc_e
	} encode;
	memset( &encode, 0, sizeof( encode ) );
	encode.depthBiasConstant = useDepthBias ? cmd->depthBiasConstant : 0;
	encode.depthBiasClamp = useDepthBias ? cmd->depthBiasClamp : 0;
	encode.depthBiasSlope = useDepthBias ? cmd->depthBiasSlope : 0;
	encode.cullMode = cullMode;
	encode.colorBlendEnabled = cmd->colorBlendEnabled;
	encode.depthWrite = cmd->depthWrite;
	// The write mask is baked into the pipeline whether or not blending is on (see colorAttachmentDesc
	// below), so it must be keyed unconditionally: masking it with colorBlendEnabled made a non-blended
	// GLSTATE_NO_COLORWRITE draw and a normal opaque draw share one slot, and whichever was built first
	// won. The blend factors, by contrast, are ignored by the GPU when blending is off, so collapsing
	// them there is correct and keeps the pipeline count down.
	encode.colorWriteMask = cmd->colorWriteMask;
	encode.colorSrcFactor = cmd->colorBlendEnabled ? cmd->colorSrcFactor : 0;
	encode.colorDstFactor = cmd->colorBlendEnabled ? cmd->colorDstFactor : 0;
	encode.topology = cmd->topology;
	encode.compareFunc = cmd->compareFunc;
	for( size_t i = 0; i < cmd->numColorsAttachments; i++ ) {
		colorAttachmentsVK[i] = RIFormatToVK( cmd->colorAttachments[i] );
	}

	hash_t hash = HASH_INITIAL_VALUE;
	hash = hash_data_hsieh( hash, vertextbindingDesc, sizeof( VkVertexInputAttributeDescription ) * vertexInputState.vertexAttributeDescriptionCount );
	hash = hash_data_hsieh( hash, vertexInputStreamsDesc, sizeof( VkVertexInputBindingDescription ) * vertexInputState.vertexBindingDescriptionCount );
	hash = hash_data_hsieh( hash, &encode, sizeof( encode ) );
	hash = hash_data_hsieh( hash, colorAttachmentsVK, sizeof( VkFormat ) * cmd->numColorsAttachments );
	if( cmd->depthFormat != RI_FORMAT_UNKNOWN )
		hash = hash_u32( hash, cmd->depthFormat );

	pipeline = __resolvePipeline( program, hash );
	assert( pipeline );
	if( pipeline->vk.handle ) {
		return pipeline; // pipeline is present in slot
	}
#elif ( DEVICE_IMPL_MTL )
	{
		(void)cullMode; // encoder state on Metal, applied in RP_BindPipeline
		// Metal folds blend state, colour write mask and attachment formats into the pipeline object, so
		// they are all part of the cache key. Cull mode, depth bias and the depth test/write flags are
		// encoder state on Metal (applied on every RP_BindPipeline) and deliberately are NOT hashed here:
		// keying them would only multiply identical MTLRenderPipelineStates.
		hash_t hash = HASH_INITIAL_VALUE;
		for( size_t i = 0; i < cmd->numAttribs; i++ ) {
			hash = hash_u32( hash, cmd->attribs[i].vk.location );
			hash = hash_u32( hash, cmd->attribs[i].offset );
			hash = hash_u32( hash, cmd->attribs[i].format );
			hash = hash_u32( hash, cmd->attribs[i].streamIndex );
		}
		for( size_t i = 0; i < cmd->numStreams; i++ ) {
			hash = hash_u32( hash, cmd->streams[i].stride );
			hash = hash_u32( hash, cmd->streams[i].bindingSlot );
		}
		for( size_t i = 0; i < cmd->numColorsAttachments; i++ )
			hash = hash_u32( hash, cmd->colorAttachments[i] );
		hash = hash_u32( hash, cmd->depthFormat );
		hash = hash_u32( hash, cmd->colorBlendEnabled );
		hash = hash_u32( hash, cmd->colorWriteMask );
		hash = hash_u32( hash, cmd->colorSrcFactor );
		hash = hash_u32( hash, cmd->colorDstFactor );
		hash = hash_u32( hash, cmd->topology );

		pipeline = __resolvePipeline( program, hash );
		assert( pipeline );
		if( pipeline->mtl.state )
			return pipeline; // pipeline is present in slot

		if( mtlc_function_is_nil( program->mtlStage[GLSL_STAGE_VERTEX].fn ) ) {
			// The program failed to cross-compile; the empty slot keeps callers from retrying every draw.
			return pipeline;
		}

		struct mtlc_vertex_descriptor vertexDesc = mtlc_vertex_descriptor_new();
		struct mtlc_vertex_attribute_descriptor_array attrArray = mtlc_vertex_descriptor_attributes( vertexDesc );
		struct mtlc_vertex_buffer_layout_descriptor_array layoutArray = mtlc_vertex_descriptor_layouts( vertexDesc );
		for( size_t i = 0; i < cmd->numAttribs; i++ ) {
			// Attributes the shader does not consume must be left out: MSL only declares the inputs it
			// uses, and describing a stage_in attribute with no matching shader input is a pipeline error.
			if( !( ( 1u << cmd->attribs[i].vk.location ) & program->vertexInputMask ) )
				continue;
			struct mtlc_vertex_attribute_descriptor attr = mtlc_vertex_attribute_descriptor_array_object( attrArray, cmd->attribs[i].vk.location );
			mtlc_vertex_attribute_descriptor_set_format( attr, RIFormatToMTLVertex( cmd->attribs[i].format ) );
			mtlc_vertex_attribute_descriptor_set_offset( attr, cmd->attribs[i].offset );
			mtlc_vertex_attribute_descriptor_set_buffer_index( attr, R_MTL_VERTEX_BUFFER_BASE + cmd->attribs[i].streamIndex );
		}
		for( size_t i = 0; i < cmd->numStreams; i++ ) {
			struct mtlc_vertex_buffer_layout_descriptor layout =
				mtlc_vertex_buffer_layout_descriptor_array_object( layoutArray, R_MTL_VERTEX_BUFFER_BASE + cmd->streams[i].bindingSlot );
			mtlc_vertex_buffer_layout_descriptor_set_stride( layout, cmd->streams[i].stride );
			mtlc_vertex_buffer_layout_descriptor_set_step_function( layout, MTLC_VERTEX_STEP_FUNCTION_PER_VERTEX );
			mtlc_vertex_buffer_layout_descriptor_set_step_rate( layout, 1 );
		}

		struct mtlc_render_pipeline_descriptor desc = mtlc_render_pipeline_descriptor_init();
		mtlc_render_pipeline_descriptor_set_vertex_function( desc, program->mtlStage[GLSL_STAGE_VERTEX].fn );
		// A pass with no colour attachment (shadow maps) must use the fragment variant with the colour
		// outputs stripped: Metal rejects a fragment function that writes color(0) into an Invalid
		// attachment format, where Vulkan just discards the write. Fall back to the full function only
		// when the variant could not be built, so the failure is reported once at pipeline creation
		// rather than hidden.
		const bool depthOnly = ( cmd->numColorsAttachments == 0 );
		bool usedDepthOnlyVariant = false;
		{
			const struct shader_mtl_stage_s *frag = &program->mtlStage[GLSL_STAGE_FRAGMENT];
			struct mtlc_function fragFn = frag->fn;
			if( depthOnly && !mtlc_function_is_nil( frag->depthOnlyFn ) ) {
				fragFn = frag->depthOnlyFn;
				usedDepthOnlyVariant = true;
			}
			if( !mtlc_function_is_nil( fragFn ) )
				mtlc_render_pipeline_descriptor_set_fragment_function( desc, fragFn );
		}
		mtlc_render_pipeline_descriptor_set_vertex_descriptor( desc, vertexDesc );

		struct mtlc_render_pipeline_color_attachment_descriptor_array colorArray = mtlc_render_pipeline_descriptor_color_attachments( desc );
		for( size_t i = 0; i < cmd->numColorsAttachments; i++ ) {
			struct mtlc_render_pipeline_color_attachment_descriptor att = mtlc_render_pipeline_color_attachment_descriptor_array_object( colorArray, i );
			mtlc_render_pipeline_color_attachment_descriptor_set_pixel_format( att, RIFormatToMTL( cmd->colorAttachments[i] ) );
			mtlc_render_pipeline_color_attachment_descriptor_set_write_mask( att, RIColorWriteMaskToMTL( cmd->colorWriteMask ) );
			mtlc_render_pipeline_color_attachment_descriptor_set_blending_enabled( att, cmd->colorBlendEnabled );
			if( cmd->colorBlendEnabled ) {
				const enum mtlc_blend_factor src = RIBlendFactorToMTL( cmd->colorSrcFactor );
				const enum mtlc_blend_factor dst = RIBlendFactorToMTL( cmd->colorDstFactor );
				mtlc_render_pipeline_color_attachment_descriptor_set_rgb_blend_factors( att, src, dst );
				mtlc_render_pipeline_color_attachment_descriptor_set_alpha_blend_factors( att, src, dst );
				mtlc_render_pipeline_color_attachment_descriptor_set_blend_operations( att, MTLC_BLEND_OPERATION_ADD, MTLC_BLEND_OPERATION_ADD );
			}
		}
		if( cmd->depthFormat != RI_FORMAT_UNKNOWN )
			mtlc_render_pipeline_descriptor_set_depth_attachment_pixel_format( desc, RIFormatToMTL( cmd->depthFormat ) );

		// Mirrors the vkSetDebugUtilsObjectNameEXT call in the VK arm; without it every pipeline shows up
		// unnamed in an Xcode GPU capture.
		if( program->name )
			mtlc_render_pipeline_descriptor_set_label( desc, ns_string_from_utf8( program->name ) );

		struct ns_error err = ns_error_from_id( NULL );
		struct mtlc_render_pipeline_state state = mtlc_device_new_render_pipeline_state( rsh.device.mtl.device, desc, &err );
		// `desc` came from _init, so it is caller-owned and has to go back on both paths. The vertex
		// descriptor does not: mtlc_vertex_descriptor_new returns an autoreleased object.
		mtlc_render_pipeline_descriptor_release( desc );
		if( mtlc_render_pipeline_state_is_nil( state ) ) {
			Com_Printf( S_COLOR_YELLOW "MTLRenderPipelineState failed (%s): %s\n", program->name ? program->name : "<unnamed>",
						ns_error_is_nil( err ) ? "unknown" : ns_string_utf8( ns_error_localized_description( err ) ) );
			return pipeline;
		}
		pipeline->mtl.state = state.obj;
		if( depthOnly )
			ri.Com_DPrintf( "Metal: depth-only pipeline for %s uses %s\n", program->name ? program->name : "<unnamed>",
							usedDepthOnlyVariant ? "the stripped-output fragment variant" : "the FULL fragment function (variant unavailable)" );
		// compareFunc/depthWrite are not part of this slot's key (see the comment above), so the depth-stencil
		// object cannot live on the pipeline -- a later draw reusing this slot with different depth state
		// would inherit the first draw's. It is resolved from a shared cache in RP_BindPipeline instead.
		return pipeline;
	}
#endif
#if ( DEVICE_IMPL_VULKAN )
	{
		uint32_t numModules = 0;
		VkShaderModule modules[4] = { 0 };
		VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		pipelineRenderingCreateInfo.colorAttachmentCount = cmd->numColorsAttachments;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = colorAttachmentsVK;
		pipelineRenderingCreateInfo.depthAttachmentFormat = RIFormatToVK( cmd->depthFormat );
		pipelineRenderingCreateInfo.stencilAttachmentFormat = GetRIFormatProps( cmd->depthFormat )->isStencil ? RIFormatToVK( cmd->depthFormat ) : VK_FORMAT_UNDEFINED;
		VkPipelineShaderStageCreateInfo stageCreateInfo[4] = { 0 };
		VkGraphicsPipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
		pipelineCreateInfo.pStages = stageCreateInfo;
		pipelineCreateInfo.basePipelineIndex = -1;
		pipelineCreateInfo.layout = program->vk.pipelineLayout;

		if( program->shaderBin[GLSL_STAGE_VERTEX].bin && program->shaderBin[GLSL_STAGE_FRAGMENT].bin ) {
			pipelineCreateInfo.stageCount = 2;
			const VkShaderModuleCreateInfo vertModuleCreateInfo = {
				VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				NULL,
				(VkShaderModuleCreateFlags)0,
				(size_t)program->shaderBin[GLSL_STAGE_VERTEX].size,
				(const uint32_t *)program->shaderBin[GLSL_STAGE_VERTEX].bin,
			};
			vkCreateShaderModule( rsh.device.vk.device, &vertModuleCreateInfo, NULL, &modules[numModules] );
			stageCreateInfo[0] =
				(VkPipelineShaderStageCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = modules[numModules], .pName = "main" };
			numModules++;

			const VkShaderModuleCreateInfo fragModuleCreateInfo = {
				VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				NULL,
				(VkShaderModuleCreateFlags)0,
				(size_t)program->shaderBin[GLSL_STAGE_FRAGMENT].size,
				(const uint32_t *)program->shaderBin[GLSL_STAGE_FRAGMENT].bin,
			};
			vkCreateShaderModule( rsh.device.vk.device, &fragModuleCreateInfo, NULL, &modules[numModules] );
			stageCreateInfo[1] = (VkPipelineShaderStageCreateInfo){
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = modules[numModules], .pName = "main" };
			numModules++;
		} else {
			assert( false && "failed to resolve bin" );
		}
		pipelineCreateInfo.pVertexInputState = &vertexInputState;

		VkPipelineColorBlendAttachmentState colorAttachmentDesc[MAX_COLOR_ATTACHMENTS] = { 0 };
		for( size_t i = 0; i < cmd->numColorsAttachments; i++ ) {
			colorAttachmentDesc[i].blendEnable = cmd->colorBlendEnabled;
			colorAttachmentDesc[i].srcColorBlendFactor = ri_vk_RIBlendFactorToVK( cmd->colorSrcFactor );
			colorAttachmentDesc[i].dstColorBlendFactor = ri_vk_RIBlendFactorToVK( cmd->colorDstFactor );
			colorAttachmentDesc[i].colorBlendOp = VK_BLEND_OP_ADD;
			// GLSTATE blend modes come from glBlendFunc, which applies the same factor pair to alpha as to
			// RGB; leaving these zero-initialised (VK_BLEND_FACTOR_ZERO) wrote alpha = 0 on every blended
			// draw with GLSTATE_ALPHAWRITE. The Metal arm sets alpha = colour factors for the same reason.
			colorAttachmentDesc[i].srcAlphaBlendFactor = colorAttachmentDesc[i].srcColorBlendFactor;
			colorAttachmentDesc[i].dstAlphaBlendFactor = colorAttachmentDesc[i].dstColorBlendFactor;
			colorAttachmentDesc[i].alphaBlendOp = VK_BLEND_OP_ADD;
			colorAttachmentDesc[i].colorWriteMask = ri_vk_RIColorWriteMaskToVK( cmd->colorWriteMask );
		}

		VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		if( cmd->numColorsAttachments > 0 ) {
			colorBlendState.attachmentCount = cmd->numColorsAttachments;
			colorBlendState.pAttachments = colorAttachmentDesc;
			pipelineCreateInfo.pColorBlendState = &colorBlendState;
		}

		VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		pipelineCreateInfo.pViewportState = &viewportState;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		inputAssemblyState.topology = ri_vk_RITopologyToVK( cmd->topology );
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;

		uint32_t dynamicStateNum = 0;
		VkDynamicState dynamicStates[16];
		dynamicStates[dynamicStateNum++] = VK_DYNAMIC_STATE_VIEWPORT;
		dynamicStates[dynamicStateNum++] = VK_DYNAMIC_STATE_SCISSOR;
		VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamicState.dynamicStateCount = dynamicStateNum;
		dynamicState.pDynamicStates = dynamicStates;
		pipelineCreateInfo.pDynamicState = &dynamicState;

		VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineCreateInfo.pMultisampleState = &multisampleState;

		VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.cullMode = ri_vk_RICullModeToVK( cullMode );
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.depthBiasEnable = useDepthBias ? VK_TRUE : VK_FALSE;
		rasterizationState.depthBiasConstantFactor = cmd->depthBiasConstant;
		rasterizationState.depthBiasClamp = cmd->depthBiasClamp;
		rasterizationState.depthBiasSlopeFactor = cmd->depthBiasSlope;
		rasterizationState.lineWidth = 1.0f;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;

		VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		depthStencilState.depthTestEnable = cmd->compareFunc != RI_COMPARE_NONE;
		depthStencilState.depthWriteEnable = cmd->depthWrite;
		depthStencilState.depthCompareOp = ri_vk_RICompareOpToVK( cmd->compareFunc );
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;

		VK_WrapResult( vkCreateGraphicsPipelines( rsh.device.vk.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &pipeline->vk.handle ) );
		//assert( ( attribFlags & program->vertexInputMask ) == program->vertexInputMask );
		if( vkSetDebugUtilsObjectNameEXT ) {
			VkDebugUtilsObjectNameInfoEXT debugName = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, NULL, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pipeline->vk.handle, program->name };
			VK_WrapResult( vkSetDebugUtilsObjectNameEXT( rsh.device.vk.device, &debugName ) );
		}

		for( size_t i = 0; i < numModules; i++ ) {
			vkDestroyShaderModule( rsh.device.vk.device, modules[i], NULL );
		}
	}
#endif

	return pipeline;
}

static const struct descriptor_reflection_s *__ReflectDescriptorSet( const struct glsl_program_s *program, const struct glsl_descriptor_handle_s *handle )
{
	for( size_t i = 0; i < program->numDescriptorReflections; i++ ) {
		if( program->descriptorReflection[i].hash == handle->hash ) {
			return &program->descriptorReflection[i];
		}
	}
	return NULL;
}

bool RP_ProgramHasUniform( const struct glsl_program_s *program, const struct glsl_descriptor_handle_s handle )
{
	if( __ReflectDescriptorSet( program, &handle ) ) {
		return true;
	}
	return false;
}

void RP_BindPushConstant( struct RIDevice_s *device, struct FrameState_s *cmd, struct glsl_program_s *program, void *data, size_t len )
{
	TracyCZoneN( ctx, "RP_BindPushConstant", 1 );
#if ( DEVICE_IMPL_VULKAN )
	{
		assert( len <= program->vk.pushConstant.size );
		vkCmdPushConstants( cmd->handle.vk.cmd, program->vk.pipelineLayout, program->vk.pushConstant.shaderStageFlags, 0, program->vk.pushConstant.size, data );
	}
#endif
#if ( DEVICE_IMPL_MTL )
	// Metal has no push-constant concept; the equivalent is inline constant data uploaded straight into
	// a buffer slot with setVertex/FragmentBytes, which avoids a scratch allocation for these few bytes.
	if( !mtlc_render_command_encoder_is_nil( cmd->handle.mtl.encoder ) && program->mtl.pushConstantSize > 0 ) {
		const struct mtlc_render_command_encoder enc = cmd->handle.mtl.encoder;
		// Upload exactly what the caller owns. The VK arm above pushes the reflected block size instead,
		// which reads past `data` whenever the caller's struct is smaller -- not a habit worth copying.
		// The assert catches the reverse (a block bigger than the struct) in debug.
		assert( len <= program->mtl.pushConstantSize );
		const mtlc_uinteger size = (mtlc_uinteger)len;
		if( program->mtl.pushConstantSlot[GLSL_STAGE_VERTEX] != R_MTL_SLOT_UNUSED )
			mtlc_render_command_encoder_set_vertex_bytes( enc, data, size, program->mtl.pushConstantSlot[GLSL_STAGE_VERTEX] );
		if( program->mtl.pushConstantSlot[GLSL_STAGE_FRAGMENT] != R_MTL_SLOT_UNUSED )
			mtlc_render_command_encoder_set_fragment_bytes( enc, data, size, program->mtl.pushConstantSlot[GLSL_STAGE_FRAGMENT] );
	}
#endif
	TracyCZoneEnd( ctx );
}

// A registerOffset past the descriptor's declared array length would land on the *next* descriptor's
// register. On Vulkan that overflows into the following binding of the same set; on Metal, where every
// set shares one flat MSL index space per resource class, it silently overwrites a descriptor from an
// unrelated set (e.g. lightmapTexture[16] spilling onto u_BaseTexture at texture(16)). Drop the write
// in both backends and say so once so the caller's loop bound gets fixed.
static bool __DescriptorOffsetOutOfRange( const struct glsl_program_s *program, const struct descriptor_reflection_s *refl,
										  const struct glsl_descriptor_binding_s *binding )
{
	const uint32_t declared = refl->dimCount ? refl->dimCount : 1;
	if( binding->registerOffset < declared )
		return false;
	// Warn once for the whole run: this is a caller bug, so one actionable line beats per-draw spam.
	static bool s_warned = false;
	if( !s_warned ) {
		s_warned = true;
		Com_Printf( S_COLOR_YELLOW "%s: descriptor '%s' bound at index %u but the shader declares only %u; skipping.\n",
					program->name ? program->name : "<unnamed>", binding->handle.name ? binding->handle.name : "<hashed>",
					binding->registerOffset, declared );
	}
	return true;
}

#if ( DEVICE_IMPL_MTL )
// The Metal binding class a runtime descriptor type belongs to -- the counterpart of __RP_MTLBindClass,
// which answers the same question from the shader's side. Used to assert the two agree.
static enum r_mtl_bind_class_e __RP_MTLBindClassFromRIType( enum RIDescriptorType_e type )
{
	switch( type ) {
		case RI_DESCRIPTOR_TYPE_SAMPLER:
			return R_MTL_BIND_SAMPLER;
		case RI_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		case RI_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			return R_MTL_BIND_TEXTURE;
		case RI_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case RI_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			return R_MTL_BIND_BUFFER;
		default:
			break;
	}
	return R_MTL_BIND_NONE;
}

// A live argument-buffer slot the frontend did not fill must still be written. Leaving it alone means
// the shader samples whichever resource the previous encode of this slot left there -- exactly the
// mis-binding argument buffers are here to stop -- so every unfilled slot gets an explicit, harmless,
// type-compatible default instead.
static struct RIDescriptor_s __RP_MTLDefaultDescriptor( const struct descriptor_reflection_s *refl )
{
	static struct RIBuffer_s s_zeroBuffer;
	switch( (enum r_mtl_bind_class_e)refl->mtlClass ) {
		case R_MTL_BIND_TEXTURE: {
			const image_t *fallback = ( refl->mtlTexDim == R_MTL_TEX_CUBE ) ? rsh.whiteCubemapTexture : rsh.whiteTexture;
			if( fallback )
				return fallback->binding;
			break;
		}
		case R_MTL_BIND_SAMPLER:
			if( rsh.whiteTexture )
				return rsh.whiteTexture->samplerBinding;
			break;
		case R_MTL_BIND_BUFFER: {
			// One zero-filled block for the whole process: a shader reading an unbound uniform block gets
			// zeroes rather than a nil buffer, which Metal treats as a fault.
			if( mtlc_buffer_is_nil( s_zeroBuffer.mtl.buffer ) ) {
				const struct RIBufferDesc_s desc = { .size = 4096, .usage = RI_BUFFER_USAGE_CONSTANT_BUFFER, .memoryLocation = RI_MEMORY_HOST_UPLOAD };
				if( InitRIBuffer( &rsh.device, &desc, &s_zeroBuffer ) == RI_SUCCESS ) {
					void *mapped = RIBufferMappedData( &rsh.device, &s_zeroBuffer );
					if( mapped )
						memset( mapped, 0, (size_t)desc.size );
				}
			}
			if( !mtlc_buffer_is_nil( s_zeroBuffer.mtl.buffer ) )
				return RIDescriptorUniformBuffer( &rsh.device, &s_zeroBuffer, 0, 4096 );
			break;
		}
		default:
			break;
	}
	return (struct RIDescriptor_s){ 0 };
}

// The descriptor the caller supplied for one element of one reflected binding, or NULL. registerOffset
// addresses an array element; it is 0 for a scalar binding.
static const struct RIDescriptor_s *__RP_MTLFindSupplied( const struct glsl_program_s *program, const struct descriptor_reflection_s *refl,
														  struct glsl_descriptor_binding_s *bindings, size_t numDescriptorData, uint32_t element )
{
	// hash 0 means the descriptor has no name in the SPIR-V (an anonymous uniform block), so no call site
	// can ever address it -- it always takes the default.
	if( refl->hash == 0 )
		return NULL;
	for( size_t i = 0; i < numDescriptorData; i++ ) {
		if( bindings[i].handle.hash != refl->hash || bindings[i].registerOffset != element )
			continue;
		if( RI_IsEmptyDescriptor( &bindings[i].descriptor ) || __DescriptorOffsetOutOfRange( program, refl, &bindings[i] ) )
			continue;
		return &bindings[i].descriptor;
	}
	return NULL;
}

// Record a resource an argument buffer now points at, so it can be declared resident. Samplers are not
// MTLResources and need none of this. The list is short (a set is a handful of entries plus at most one
// 16-element array), so a linear dedup beats any structure.
static void __RP_MTLTrackResident( struct descriptor_set_slot_s *slot, void *resource )
{
	if( !resource )
		return;
	for( size_t i = 0; i < (size_t)arrlen( slot->mtl.residentResources ); i++ ) {
		if( slot->mtl.residentResources[i] == resource )
			return;
	}
	arrpush( slot->mtl.residentResources, resource );
}
#endif

void RP_BindDescriptorSets( struct RIDevice_s *device, struct FrameState_s *cmd, struct glsl_program_s *program, struct glsl_descriptor_binding_s *bindings, size_t numDescriptorData )
{
	TracyCZoneN( ctx, "RP_BindDescriptorSets", 1 );
#if ( DEVICE_IMPL_VULKAN )
	{
		size_t numWrites = 0;
		VkWriteDescriptorSet descriptorWrite[32]; // write 32 descriptors at once
		// Parallel storage for acceleration-structure writes: the VkWriteDescriptorSetAccelerationStructureKHR
		// chained via pNext must outlive the batch, so it is kept at the same index as its descriptorWrite entry.
		VkWriteDescriptorSetAccelerationStructureKHR accelWrites[32];
		for( uint32_t setIndex = 0; setIndex < R_DESCRIPTOR_SET_MAX; setIndex++ ) {
			hash_t hash = HASH_INITIAL_VALUE;
			for( size_t i = 0; i < numDescriptorData; i++ ) {
				const struct descriptor_reflection_s *refl = __ReflectDescriptorSet( program, &bindings[i].handle );
				if( !refl || setIndex != refl->set || RI_IsEmptyDescriptor( &bindings[i].descriptor ) ||
					__DescriptorOffsetOutOfRange( program, refl, &bindings[i] ) )
					continue;
				hash = hash_u64( hash, refl->hash );
				assert(bindings[i].descriptor.cookie != 0); // the cookie can't be 0
				hash = hash_u64( hash, bindings[i].descriptor.cookie );
				// Fold in the array element: the same descriptor bound at a different dstArrayElement
				// is a distinct set contents, so it must not hash to (and reuse) the same cached set.
				hash = hash_u64( hash, bindings[i].registerOffset );
			}
			if( hash == HASH_INITIAL_VALUE )
				continue;
			struct glsl_program_descriptor_s *info = &program->programDescriptors[setIndex];
			struct descriptor_set_result_s result = ResolveDescriptorSet( &rsh.device, &info->alloc, rsh.frameSetCount, hash );
			if( !result.found ) {
				for( size_t i = 0; i < numDescriptorData; i++ ) {
					const struct descriptor_reflection_s *refl = __ReflectDescriptorSet( program, &bindings[i].handle );
					if( !refl || setIndex != refl->set || RI_IsEmptyDescriptor( &bindings[i].descriptor ) ||
					__DescriptorOffsetOutOfRange( program, refl, &bindings[i] ) )
						continue;

					if( numWrites == Q_ARRAY_COUNT( descriptorWrite ) ) {
						vkUpdateDescriptorSets( device->vk.device, numWrites, descriptorWrite, 0, NULL );
						numWrites = 0;
					}

					assert( numWrites < Q_ARRAY_COUNT( descriptorWrite ) );
					const size_t writeIdx = numWrites;
					VkWriteDescriptorSet *vkDesc = descriptorWrite + ( numWrites++ );
					memset( vkDesc, 0, sizeof( VkWriteDescriptorSet ) );
					vkDesc->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					vkDesc->dstSet = result.set->vk.handle;
					if( refl->isArray ) {
						vkDesc->dstBinding = refl->baseRegisterIndex;
						vkDesc->dstArrayElement = bindings[i].registerOffset;
					} else {
						vkDesc->dstBinding = refl->baseRegisterIndex + bindings[i].registerOffset;
						vkDesc->dstArrayElement = 0;
					}
					vkDesc->descriptorCount = 1;
					vkDesc->descriptorType = RI_VK_BindlessDescriptorType( bindings[i].descriptor.type );
					switch( (enum RIDescriptorType_e)bindings[i].descriptor.type ) {
						case RI_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
						case RI_DESCRIPTOR_TYPE_STORAGE_BUFFER:
							vkDesc->pBufferInfo = &bindings[i].descriptor.vk.buffer;
							break;
						case RI_DESCRIPTOR_TYPE_SAMPLER:
						case RI_DESCRIPTOR_TYPE_STORAGE_IMAGE:
						case RI_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
							vkDesc->pImageInfo = &bindings[i].descriptor.vk.image;
							break;
						case RI_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE: {
							VkWriteDescriptorSetAccelerationStructureKHR *accel = &accelWrites[writeIdx];
							memset( accel, 0, sizeof( *accel ) );
							accel->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
							accel->accelerationStructureCount = 1;
							accel->pAccelerationStructures = &bindings[i].descriptor.vk.accelStructure;
							vkDesc->pNext = accel;
							break;
						}
						default:
							assert( false ); // this is bad
							break;
					}

				}
			}
			if( numWrites > 0 ) {
				vkUpdateDescriptorSets( device->vk.device, numWrites, descriptorWrite, 0, NULL );
				numWrites = 0;
			}
			VkDescriptorSet vkDescriptorSet = result.set->vk.handle;
			vkCmdBindDescriptorSets( cmd->handle.vk.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, program->vk.pipelineLayout, setIndex, 1, &vkDescriptorSet, 0, NULL );
		}
	}
#endif
#if ( DEVICE_IMPL_MTL )
	{
		struct mtlc_render_command_encoder enc = cmd->handle.mtl.encoder;
		if( mtlc_render_command_encoder_is_nil( enc ) ) {
			TracyCZoneEnd( ctx );
			return;
		}
		for( uint32_t setIndex = 0; setIndex < R_DESCRIPTOR_SET_MAX; setIndex++ ) {
			struct glsl_program_descriptor_s *programDesc = &program->programDescriptors[setIndex];

			// -- Discrete set: bind each supplied resource straight onto the encoder ------------------
			// These sets (see R_MTL_SET_IS_ARGUMENT_BUFFER) are per-draw scratch UBOs, so there is nothing
			// to cache and one setBuffer per descriptor is already the cheapest thing available.
			if( programDesc->mtl.argBufferIndex == R_MTL_SLOT_UNUSED ) {
				for( size_t i = 0; i < numDescriptorData; i++ ) {
					const struct descriptor_reflection_s *refl = __ReflectDescriptorSet( program, &bindings[i].handle );
					if( !refl || refl->set != setIndex || RI_IsEmptyDescriptor( &bindings[i].descriptor ) ||
						__DescriptorOffsetOutOfRange( program, refl, &bindings[i] ) )
						continue;
					const struct RIDescriptor_s *d = &bindings[i].descriptor;
					for( glsl_program_stage_t stage = 0; stage < GLSL_STAGE_MAX; stage++ ) {
						if( refl->mtlSlot[stage] == R_MTL_SLOT_UNUSED )
							continue;
						const uint8_t slot = (uint8_t)( refl->mtlSlot[stage] + bindings[i].registerOffset );
						switch( (enum RIDescriptorType_e)d->type ) {
							case RI_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
							case RI_DESCRIPTOR_TYPE_STORAGE_BUFFER:
								if( stage == GLSL_STAGE_VERTEX )
									mtlc_render_command_encoder_set_vertex_buffer( enc, d->mtl.buffer, d->mtl.offset, slot );
								else if( stage == GLSL_STAGE_FRAGMENT )
									mtlc_render_command_encoder_set_fragment_buffer( enc, d->mtl.buffer, d->mtl.offset, slot );
								break;
							case RI_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
							case RI_DESCRIPTOR_TYPE_STORAGE_IMAGE:
								if( stage == GLSL_STAGE_VERTEX )
									mtlc_render_command_encoder_set_vertex_texture( enc, d->mtl.texture, slot );
								else if( stage == GLSL_STAGE_FRAGMENT )
									mtlc_render_command_encoder_set_fragment_texture( enc, d->mtl.texture, slot );
								break;
							case RI_DESCRIPTOR_TYPE_SAMPLER:
								if( stage == GLSL_STAGE_VERTEX )
									mtlc_render_command_encoder_set_vertex_sampler_state( enc, mtlc_sampler_state_from_id( d->mtl.sampler ), slot );
								else if( stage == GLSL_STAGE_FRAGMENT )
									mtlc_render_command_encoder_set_fragment_sampler_state( enc, mtlc_sampler_state_from_id( d->mtl.sampler ), slot );
								break;
							default:
								break;
						}
					}
				}
				continue;
			}

			// -- Argument-buffer set ------------------------------------------------------------------
			// One encoded table, resolved from the same content-hash cache the Vulkan arm uses, bound with
			// a single setBuffer. Note this set is bound unconditionally: the program declares it, so the
			// shader reads it, and skipping the bind would leave whatever the previously bound program left
			// at this buffer index.
			hash_t hash = hash_u64( HASH_INITIAL_VALUE, RIResourceEpoch() );
			for( size_t i = 0; i < numDescriptorData; i++ ) {
				const struct descriptor_reflection_s *refl = __ReflectDescriptorSet( program, &bindings[i].handle );
				if( !refl || refl->set != setIndex || RI_IsEmptyDescriptor( &bindings[i].descriptor ) ||
					__DescriptorOffsetOutOfRange( program, refl, &bindings[i] ) )
					continue;
				hash = hash_u64( hash, refl->hash );
				assert( bindings[i].descriptor.cookie != 0 ); // the cookie can't be 0
				hash = hash_u64( hash, bindings[i].descriptor.cookie );
				hash = hash_u64( hash, bindings[i].registerOffset );
			}

			struct descriptor_set_result_s result = ResolveDescriptorSet( &rsh.device, &programDesc->alloc, rsh.frameSetCount, hash );
			if( !result.set || mtlc_buffer_is_nil( result.set->mtl.buffer ) )
				continue; // pool allocation failed; already reported

			uint32_t stageMask = 0;
			if( !result.found ) {
				// A recycled slot still holds the previous tenant's table, so every reflected entry is
				// rewritten -- supplied or defaulted -- not just the ones this draw provided.
				if( result.set->mtl.residentResources )
					arrsetlen( result.set->mtl.residentResources, 0 );
				for( glsl_program_stage_t stage = 0; stage < GLSL_STAGE_MAX; stage++ ) {
					if( programDesc->mtl.stageLength[stage] == 0 )
						continue;
					struct mtlc_argument_encoder argEncoder = program->mtlStage[stage].argEncoder[setIndex];
					mtlc_argument_encoder_set_argument_buffer( argEncoder, result.set->mtl.buffer,
															   result.set->mtl.offset + programDesc->mtl.stageOffset[stage] );
					for( size_t r = 0; r < program->numDescriptorReflections; r++ ) {
						const struct descriptor_reflection_s *refl = &program->descriptorReflection[r];
						if( refl->set != setIndex || refl->mtlSlot[stage] == R_MTL_SLOT_UNUSED )
							continue;
						const uint32_t count = refl->dimCount ? refl->dimCount : 1;
						// An unfilled element of an arrayed binding borrows whichever element the caller did
						// supply. That is guaranteed to be the type the shader declares -- shadowmapTexture[4]
						// is a depth2d array that a draw fills only up to its shadow count, and the white RGBA
						// texture the generic default hands out is the wrong texture type for those slots --
						// and the shader never reads past the elements it was given. Only a binding with no
						// element at all falls back to the generic default.
						const struct RIDescriptor_s *anySupplied = NULL;
						for( uint32_t element = 0; element < count && !anySupplied; element++ )
							anySupplied = __RP_MTLFindSupplied( program, refl, bindings, numDescriptorData, element );
						for( uint32_t element = 0; element < count; element++ ) {
							const struct RIDescriptor_s *supplied = __RP_MTLFindSupplied( program, refl, bindings, numDescriptorData, element );
							if( !supplied )
								supplied = anySupplied;
							const struct RIDescriptor_s fallback = supplied ? (struct RIDescriptor_s){ 0 } : __RP_MTLDefaultDescriptor( refl );
							const struct RIDescriptor_s *d = supplied ? supplied : &fallback;
							if( RI_IsEmptyDescriptor( d ) )
								continue; // no default available yet (pre-registration); leave the slot alone
							// SPIRV-Cross reserved dimCount consecutive ids for an arrayed resource, so an
							// element lands at (base id + its index) -- verified against the encoded bytes.
							const mtlc_uinteger id = (mtlc_uinteger)( refl->mtlSlot[stage] + element );
							// The id comes from what the *shader* declared but the payload from what the
							// caller passed. RIDescriptor_s is a union, so a mismatch would encode e.g. a
							// sampler pointer into a texture slot with no complaint from anyone. Skip rather
							// than write nonsense; the assert catches the call site in a debug build.
							assert( refl->mtlClass == (uint8_t)__RP_MTLBindClassFromRIType( (enum RIDescriptorType_e)d->type ) );
							if( refl->mtlClass != (uint8_t)__RP_MTLBindClassFromRIType( (enum RIDescriptorType_e)d->type ) )
								continue;
							switch( (enum r_mtl_bind_class_e)refl->mtlClass ) {
								case R_MTL_BIND_TEXTURE:
									mtlc_argument_encoder_set_texture( argEncoder, d->mtl.texture, id );
									__RP_MTLTrackResident( result.set, d->mtl.texture.obj );
									break;
								case R_MTL_BIND_SAMPLER:
									mtlc_argument_encoder_set_sampler_state( argEncoder, mtlc_sampler_state_from_id( d->mtl.sampler ), id );
									break; // a sampler state is not an MTLResource; nothing to make resident
								case R_MTL_BIND_BUFFER:
									mtlc_argument_encoder_set_buffer( argEncoder, d->mtl.buffer, d->mtl.offset, id );
									__RP_MTLTrackResident( result.set, d->mtl.buffer.obj );
									break;
								default:
									break;
							}
						}
					}
				}
				// On a discrete GPU the pool buffer is Managed, so the encoded bytes have to be published.
				if( !rsh.device.physicalAdapter.mtl.hasUnifiedMemory )
					mtlc_buffer_did_modify_range( result.set->mtl.buffer,
												  (struct ns_range){ result.set->mtl.offset, programDesc->mtl.stride } );
				result.set->mtl.residentEpoch = 0; // force the residency declaration below
			}

			for( glsl_program_stage_t stage = 0; stage < GLSL_STAGE_MAX; stage++ ) {
				if( programDesc->mtl.stageLength[stage] == 0 )
					continue;
				stageMask |= ( stage == GLSL_STAGE_VERTEX ) ? MTLC_RENDER_STAGE_VERTEX : MTLC_RENDER_STAGE_FRAGMENT;
			}

			// Resources reached through an argument buffer are outside Metal's automatic residency
			// tracking. Declaring them is per-encoder state, so this is skipped for a slot already declared
			// to this encoder -- which, once the cache is warm, is the common case.
			if( result.set->mtl.residentEpoch != cmd->handle.mtl.encoderEpoch ) {
				const size_t numResident = (size_t)arrlen( result.set->mtl.residentResources );
				if( numResident > 0 )
					mtlc_render_command_encoder_use_resources( enc, result.set->mtl.residentResources, (mtlc_uinteger)numResident,
															   MTLC_RESOURCE_USAGE_READ, stageMask );
				result.set->mtl.residentEpoch = cmd->handle.mtl.encoderEpoch;
			}

			for( glsl_program_stage_t stage = 0; stage < GLSL_STAGE_MAX; stage++ ) {
				if( programDesc->mtl.stageLength[stage] == 0 )
					continue;
				const mtlc_uinteger offset = result.set->mtl.offset + programDesc->mtl.stageOffset[stage];
				if( stage == GLSL_STAGE_VERTEX )
					mtlc_render_command_encoder_set_vertex_buffer( enc, result.set->mtl.buffer, offset, programDesc->mtl.argBufferIndex );
				else if( stage == GLSL_STAGE_FRAGMENT )
					mtlc_render_command_encoder_set_fragment_buffer( enc, result.set->mtl.buffer, offset, programDesc->mtl.argBufferIndex );
			}
		}
	}
#endif
	TracyCZoneEnd( ctx );
}

struct glsl_program_s *RP_ResolveProgram( int type, const char *name, const char *deformsKey, const deformv_t *deforms, int numDeforms, r_glslfeat_t features )
{
	TracyCZoneN( ctx, "RP_ResolveProgram", 1 );
	assert( type > GLSL_PROGRAM_TYPE_NONE && type < GLSL_PROGRAM_TYPE_MAXTYPE );
	assert( !deforms || deformsKey );

	if( !deforms )
		deformsKey = "";

	const uint64_t hashIndex = hash_u64( HASH_INITIAL_VALUE, features ) % GLSL_PROGRAMS_HASH_SIZE;
	for( struct glsl_program_s *program = r_glslprograms_hash[type][hashIndex]; program; program = program->hash_next ) {
		if( ( program->features == features ) && strcmp( program->deformsKey, deformsKey ) == 0 ) {
			TracyCZoneEnd( ctx );
			return program;
		}
	}

	if( r_numglslprograms == MAX_GLSL_PROGRAMS ) {
		Com_Printf( S_COLOR_YELLOW "RP_RegisterProgram: GLSL programs limit exceeded\n" );
		TracyCZoneEnd( ctx );
		return NULL;
	}

	struct glsl_program_s *program;
	if( !name ) {
		struct glsl_program_s *parent = NULL;
		for( size_t i = 0; i < r_numglslprograms; i++ ) {
			program = r_glslprograms + i;

			if( ( program->type == type ) && !program->features ) {
				parent = program;
				break;
			}
		}

		if( parent ) {
			if( !name )
				name = parent->name;
		} else {
			Com_Printf( S_COLOR_YELLOW "RP_RegisterProgram: failed to find parent for program type %i\n", type );
			TracyCZoneEnd( ctx );
			return 0;
		}
	}

	{
		struct glsl_program_s *ret = RP_RegisterProgram( type, name, deformsKey, deforms, numDeforms, features );
		TracyCZoneEnd( ctx );
		return ret;
	}
}

#if ( DEVICE_IMPL_VULKAN )
void _vk__descriptorSetAlloc( struct RIDevice_s *device, struct DescriptorSetAllocator *alloc )
{
	assert( device->renderer->api == RI_DEVICE_API_VK );
	struct glsl_program_descriptor_s *programDescriptor = Q_CONTAINER_OF( alloc, struct glsl_program_descriptor_s, alloc );
	VkDescriptorPoolSize descriptorPoolSize[16] = { 0 };
	size_t descriptorPoolLen = 0;
	if( programDescriptor->samplerMaxNum > 0 )
		descriptorPoolSize[descriptorPoolLen++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_SAMPLER, programDescriptor->samplerMaxNum * DESCRIPTOR_MAX_SIZE };
	if( programDescriptor->constantBufferMaxNum > 0 )
		descriptorPoolSize[descriptorPoolLen++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, programDescriptor->constantBufferMaxNum * DESCRIPTOR_MAX_SIZE };
	if( programDescriptor->dynamicConstantBufferMaxNum > 0 )
		descriptorPoolSize[descriptorPoolLen++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, programDescriptor->dynamicConstantBufferMaxNum * DESCRIPTOR_MAX_SIZE };
	if( programDescriptor->textureMaxNum > 0 )
		descriptorPoolSize[descriptorPoolLen++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, programDescriptor->textureMaxNum * DESCRIPTOR_MAX_SIZE };
	if( programDescriptor->storageTextureMaxNum > 0 )
		descriptorPoolSize[descriptorPoolLen++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, programDescriptor->storageTextureMaxNum * DESCRIPTOR_MAX_SIZE };
	if( programDescriptor->bufferMaxNum > 0 )
		descriptorPoolSize[descriptorPoolLen++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, programDescriptor->bufferMaxNum * DESCRIPTOR_MAX_SIZE };
	if( programDescriptor->storageBufferMaxNum > 0 )
		descriptorPoolSize[descriptorPoolLen++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, programDescriptor->storageBufferMaxNum * DESCRIPTOR_MAX_SIZE };
	if( programDescriptor->structuredBufferMaxNum > 0 || programDescriptor->storageStructuredBufferMaxNum > 0 )
		descriptorPoolSize[descriptorPoolLen++] = (VkDescriptorPoolSize){
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, programDescriptor->structuredBufferMaxNum * DESCRIPTOR_MAX_SIZE + programDescriptor->storageStructuredBufferMaxNum * DESCRIPTOR_MAX_SIZE };
	if( programDescriptor->accelerationStructureMaxNum > 0 )
		descriptorPoolSize[descriptorPoolLen++] = (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, programDescriptor->accelerationStructureMaxNum * DESCRIPTOR_MAX_SIZE };
	assert( descriptorPoolLen <= Q_ARRAY_COUNT( descriptorPoolSize ) );
	const VkDescriptorPoolCreateInfo info = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, NULL, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, DESCRIPTOR_MAX_SIZE, descriptorPoolLen, descriptorPoolSize };
	struct descriptor_pool_alloc_slot_s poolSlot = { 0 };
	VK_WrapResult( vkCreateDescriptorPool( device->vk.device, &info, NULL, &poolSlot.vk.handle ) );
	arrpush( alloc->pools, poolSlot );

	for( size_t i = 0; i < DESCRIPTOR_MAX_SIZE; i++ ) {
		struct descriptor_set_slot_s *slot = AllocDescriptorsetSlot( alloc );
		VkDescriptorSetAllocateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		info.pNext = NULL;
		info.descriptorPool = poolSlot.vk.handle;
		info.descriptorSetCount = 1;
		assert( programDescriptor->vk.setLayout != VK_NULL_HANDLE );
		info.pSetLayouts = &programDescriptor->vk.setLayout;
		VK_WrapResult( vkAllocateDescriptorSets( device->vk.device, &info, &slot->vk.handle ) );
		arrpush( alloc->reservedSlots, slot );
	}
}

#endif


#if ( DEVICE_IMPL_MTL )

// One "pool" is a single buffer carved into DESCRIPTOR_MAX_SIZE argument-buffer slots. Metal has no
// descriptor-pool object, so unlike the Vulkan arm there is nothing to size by descriptor type -- the
// stride is whatever MTLArgumentEncoder reported for this set, which __RP_MTLBuildLibraries has already
// resolved by the time any draw can reach here.
void _mtl__descriptorSetAlloc( struct RIDevice_s *device, struct DescriptorSetAllocator *alloc )
{
	struct glsl_program_descriptor_s *programDescriptor = Q_CONTAINER_OF( alloc, struct glsl_program_descriptor_s, alloc );
	assert( programDescriptor->mtl.stride > 0 );

	struct RIBufferDesc_s desc = {
		.size = (uint64_t)programDescriptor->mtl.stride * DESCRIPTOR_MAX_SIZE,
		.usage = RI_BUFFER_USAGE_ARGUMENT_BUFFER,
		.memoryLocation = RI_MEMORY_HOST_UPLOAD, // the encoder writes from the CPU
	};
	struct RIBuffer_s buffer = { 0 };
	if( InitRIBuffer( device, &desc, &buffer ) != RI_SUCCESS )
		Com_Printf( S_COLOR_YELLOW "Metal: argument-buffer pool allocation failed (%u bytes)\n", (unsigned)desc.size );

	// Slots are handed out even on failure: ResolveDescriptorSet pops one unconditionally after calling
	// here, so returning empty-handed would pop an empty array. A nil buffer is caught at encode instead.
	if( !mtlc_buffer_is_nil( buffer.mtl.buffer ) ) {
		struct descriptor_pool_alloc_slot_s poolSlot = { 0 };
		poolSlot.mtl.buffer = buffer.mtl.buffer;
		arrpush( alloc->pools, poolSlot );
	}

	for( size_t i = 0; i < DESCRIPTOR_MAX_SIZE; i++ ) {
		struct descriptor_set_slot_s *slot = AllocDescriptorsetSlot( alloc );
		slot->mtl.buffer = buffer.mtl.buffer;
		slot->mtl.offset = (uint32_t)( i * programDescriptor->mtl.stride );
		slot->mtl.residentEpoch = 0;
		arrpush( alloc->reservedSlots, slot );
	}
}

static enum r_mtl_bind_class_e __RP_MTLBindClass( SpvReflectDescriptorType type )
{
	switch( type ) {
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
			return R_MTL_BIND_SAMPLER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			return R_MTL_BIND_TEXTURE;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			return R_MTL_BIND_BUFFER;
		default:
			break;
	}
	return R_MTL_BIND_NONE;
}

static SpvExecutionModel __RP_MTLExecutionModel( glsl_program_stage_t stage )
{
	switch( stage ) {
		case GLSL_STAGE_VERTEX:
			return SpvExecutionModelVertex;
		case GLSL_STAGE_FRAGMENT:
			return SpvExecutionModelFragment;
		default:
			break;
	}
	assert( false );
	return SpvExecutionModelVertex;
}

// Reflect every stage's SPIR-V and fill the backend-neutral fields the Vulkan path fills inside its own
// (much larger) VkDescriptorSetLayout-building block: push-constant presence/size, the vertex input mask,
// the per-set resource counts, and the name-hash -> descriptor reflection table. Slot assignment for MSL
// happens here too, because SPIRV-Cross needs the bindings pinned *before* it emits the shader.
static bool __RP_MTLReflectProgram( struct glsl_program_s *program, const glsl_program_stage_t *stageList, size_t numStages )
{
	// Per-stage, per-class running slot counters. Index order matches enum r_mtl_bind_class_e.
	uint8_t nextSlot[GLSL_STAGE_MAX][3] = { 0 };
	SpvReflectDescriptorSet **descSets = NULL;
	bool error = false;

	for( size_t i = 0; i < numStages; i++ ) {
		const glsl_program_stage_t stage = stageList[i];
		const struct shader_bin_data_s *bin = &program->shaderBin[stage];
		if( !bin->bin || bin->size == 0 )
			continue;

		SpvReflectShaderModule module = { 0 };
		SpvReflectResult result = spvReflectCreateShaderModule( bin->size, bin->bin, &module );
		if( result != SPV_REFLECT_RESULT_SUCCESS ) {
			Com_Printf( S_COLOR_YELLOW "SPIRV-Reflect failed for %s stage %d\n", program->name ? program->name : "<unnamed>", (int)stage );
			error = true;
			break;
		}

		{
			uint32_t pushConstantCount = 0;
			result = spvReflectEnumeratePushConstantBlocks( &module, &pushConstantCount, NULL );
			assert( result == SPV_REFLECT_RESULT_SUCCESS );
			if( pushConstantCount > 1 ) {
				Com_Printf( S_COLOR_YELLOW "Push constant count is greater than 1, only supporting 1 push constant\n" );
				spvReflectDestroyShaderModule( &module );
				error = true;
				break;
			}
			program->hasPushConstant |= ( pushConstantCount > 0 );
			if( pushConstantCount > 0 ) {
				// Only the size is taken here; the MSL buffer index it lands on is read back from
				// SPIRV-Cross in __RP_MTLCompileToMSL, alongside the descriptor slots. Stages may declare
				// different-sized blocks, so keep the largest -- the bind uploads that many bytes and a
				// stage then never reads past what was written.
				SpvReflectBlockVariable *block = NULL;
				result = spvReflectEnumeratePushConstantBlocks( &module, &pushConstantCount, &block );
				assert( result == SPV_REFLECT_RESULT_SUCCESS );
				if( block && block->size > program->mtl.pushConstantSize )
					program->mtl.pushConstantSize = block->size;
			}
		}

		if( stage == GLSL_STAGE_VERTEX ) {
			for( uint32_t v = 0; v < module.input_variable_count; v++ ) {
				// Built-ins report location ~0u and are not real vertex attributes.
				if( module.input_variables[v]->location >= MAX_ATTRIBUTES )
					continue;
				program->vertexInputMask |= ( 1u << module.input_variables[v]->location );
			}
		}

		uint32_t setCount = 0;
		result = spvReflectEnumerateDescriptorSets( &module, &setCount, NULL );
		assert( result == SPV_REFLECT_RESULT_SUCCESS );
		arrsetlen( descSets, setCount );
		result = spvReflectEnumerateDescriptorSets( &module, &setCount, descSets );
		assert( result == SPV_REFLECT_RESULT_SUCCESS );

		for( uint32_t s = 0; s < setCount; s++ ) {
			const SpvReflectDescriptorSet *set = descSets[s];
			assert( set->set < R_DESCRIPTOR_SET_MAX );
			struct glsl_program_descriptor_s *programDesc = &program->programDescriptors[set->set];
			for( uint32_t b = 0; b < set->binding_count; b++ ) {
				const SpvReflectDescriptorBinding *binding = set->bindings[b];
				assert( binding->array.dims_count <= 1 ); // not going to handle multi-dim arrays
				const enum r_mtl_bind_class_e cls = __RP_MTLBindClass( binding->descriptor_type );
				if( cls == R_MTL_BIND_NONE ) {
					Com_Printf( S_COLOR_YELLOW "Unsupported descriptor type %d on Metal (%s)\n", (int)binding->descriptor_type, binding->name ? binding->name : "" );
					continue;
				}
				const uint32_t bindingCount = max( binding->count, 1 );
				const hash_t hash = Create_DescriptorHandle( binding->name ).hash;

				// A descriptor shared by both stages is one table entry with a slot recorded per stage.
				// Identity is (set, binding), not the name hash: an anonymous uniform block reports no name
				// (hash 0), so keying on the name would collapse every such block onto one entry and hand
				// the second one the first one's set/binding -- and the MSL slot readback below matches on
				// (set, binding) too, so the two must agree.
				struct descriptor_reflection_s *reflc = NULL;
				for( size_t r = 0; r < program->numDescriptorReflections; r++ ) {
					if( program->descriptorReflection[r].set == binding->set &&
						program->descriptorReflection[r].baseRegisterIndex == binding->binding ) {
						reflc = &program->descriptorReflection[r];
						break;
					}
				}
				if( !reflc ) {
					assert( program->numDescriptorReflections < PIPELINE_REFLECTION_HASH_SIZE );
					if( program->numDescriptorReflections >= PIPELINE_REFLECTION_HASH_SIZE ) {
						spvReflectDestroyShaderModule( &module );
						error = true;
						goto reflect_done;
					}
					reflc = &program->descriptorReflection[program->numDescriptorReflections++];
					memset( reflc, 0, sizeof( *reflc ) );
					reflc->hash = hash;
					reflc->set = binding->set;
					reflc->baseRegisterIndex = binding->binding;
					reflc->isArray = binding->count > 1;
					reflc->dimCount = bindingCount;
					reflc->mtlClass = (uint8_t)cls;
					reflc->mtlTexDim = ( binding->image.dim == SpvDimCube ) ? R_MTL_TEX_CUBE : R_MTL_TEX_2D;
					memset( reflc->mtlSlot, R_MTL_SLOT_UNUSED, sizeof( reflc->mtlSlot ) );

					// Per-set pool sizing is shared with the Vulkan path; only count a binding once.
					switch( binding->descriptor_type ) {
						case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:               programDesc->samplerMaxNum += bindingCount; break;
						case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:         programDesc->textureMaxNum += bindingCount; break;
						case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:  programDesc->bufferMaxNum += bindingCount; break;
						case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:         programDesc->storageTextureMaxNum += bindingCount; break;
						case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:  programDesc->storageBufferMaxNum += bindingCount; break;
						case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:        programDesc->constantBufferMaxNum += bindingCount; break;
						case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
						case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: programDesc->structuredBufferMaxNum += bindingCount; break;
						default: break;
					}
				}
				assert( reflc->mtlClass == (uint8_t)cls );
				if( reflc->mtlSlot[stage] == R_MTL_SLOT_UNUSED ) {
					reflc->mtlSlot[stage] = nextSlot[stage][cls];
					nextSlot[stage][cls] += (uint8_t)bindingCount;
				}
			}
		}
		spvReflectDestroyShaderModule( &module );
	}

reflect_done:
	arrfree( descSets );
	if( error )
		return false;

	// Decide, per set, whether it becomes an argument buffer, and pin the MSL buffer index it will bind
	// at. This has to happen before any cross-compile: the decision is an input to SPIRV-Cross, and both
	// stages must agree, so it is derived from the program-wide union of both stages' reflections.
	{
		const mtlc_uinteger samplerCap = mtlc_device_max_argument_buffer_sampler_count( rsh.device.mtl.device );
		uint8_t nextArgBuffer = R_MTL_ARG_BUFFER_BASE;
		for( uint32_t set = 0; set < R_DESCRIPTOR_SET_MAX; set++ ) {
			struct glsl_program_descriptor_s *programDesc = &program->programDescriptors[set];
			programDesc->mtl.argBufferIndex = R_MTL_SLOT_UNUSED;

			bool used = false;
			for( size_t r = 0; r < program->numDescriptorReflections && !used; r++ )
				used = ( program->descriptorReflection[r].set == set );
			if( !used )
				continue;

			// A set is only worth an argument buffer if its contents are stable enough to hit the content
			// cache. Every buffer this renderer binds is a per-draw suballocation from the frame scratch
			// ring, and UpdateFrameUBO salts its cookie with the frame counter, so any set holding one can
			// never hit -- it would re-encode a table every draw in place of a handful of direct setBuffer
			// calls. Texture/sampler sets, whose contents are stable for the life of a map, are the win.
			if( programDesc->constantBufferMaxNum || programDesc->dynamicConstantBufferMaxNum || programDesc->bufferMaxNum ||
				programDesc->storageBufferMaxNum || programDesc->structuredBufferMaxNum || programDesc->storageStructuredBufferMaxNum ) {
				ri.Com_DPrintf( "%s: set %u holds buffers; keeping it off argument buffers\n", program->name ? program->name : "<unnamed>", set );
				continue;
			}

			// Tier 1 caps how many samplers may be reachable through argument buffers at once. A set over
			// the cap stays discrete rather than failing at draw time.
			if( programDesc->samplerMaxNum > samplerCap ) {
				ri.Com_DPrintf( "%s: set %u has %u samplers (device cap %u); keeping it off argument buffers\n",
							 program->name ? program->name : "<unnamed>", set, (unsigned)programDesc->samplerMaxNum, (unsigned)samplerCap );
				continue;
			}

			programDesc->mtl.argBufferIndex = nextArgBuffer++;
			programDesc->alloc.descriptorAllocator = _mtl__descriptorSetAlloc;
			programDesc->alloc.framesInFlight = NUMBER_FRAMES_FLIGHT;
		}
	}
	return true;
}

// All fragment outputs enabled -- the SPIRV-Cross default for enable_frag_output_mask, and the value
// that marks a compile as the primary one whose slot read-back is recorded.
#define R_MTL_FRAG_OUTPUT_ALL 0xffffffffu

// SPIR-V -> MSL for one stage. With fragOutputMask == R_MTL_FRAG_OUTPUT_ALL this is the primary compile
// and it records the MSL resource index SPIRV-Cross assigns each descriptor into
// program->descriptorReflection[].mtlSlot[stage] (hence non-const program). Any other mask compiles a
// variant with those fragment colour outputs stripped (depth-only passes); a variant must land every
// resource on the slot the primary recorded -- RP_BindDescriptorSets binds one slot table per stage --
// so instead of recording it verifies, and fails if the two disagree. Returns a heap-allocated MSL
// string the caller owns, or NULL on failure.
static char *__RP_MTLCompileToMSL( struct glsl_program_s *program, glsl_program_stage_t stage, uint32_t fragOutputMask )
{
	const bool recordSlots = ( fragOutputMask == R_MTL_FRAG_OUTPUT_ALL );
	const struct shader_bin_data_s *bin = &program->shaderBin[stage];
	if( !bin->bin || bin->size == 0 )
		return NULL;

	spvc_context context = NULL;
	spvc_parsed_ir ir = NULL;
	spvc_compiler compiler = NULL;
	spvc_compiler_options options = NULL;
	const char *msl = NULL;
	char *out = NULL;

	if( spvc_context_create( &context ) != SPVC_SUCCESS )
		return NULL;

	if( spvc_context_parse_spirv( context, (const SpvId *)bin->bin, bin->size / sizeof( uint32_t ), &ir ) != SPVC_SUCCESS )
		goto fail;
	if( spvc_context_create_compiler( context, SPVC_BACKEND_MSL, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler ) != SPVC_SUCCESS )
		goto fail;
	if( spvc_compiler_create_compiler_options( compiler, &options ) != SPVC_SUCCESS )
		goto fail;

	// MSL 2.1 is the floor for every Metal-capable Mac this engine targets, and is above the 2.0 argument
	// buffers require.
	spvc_compiler_options_set_uint( options, SPVC_COMPILER_OPTION_MSL_VERSION, SPVC_MAKE_MSL_VERSION( 2, 1, 0 ) );
	spvc_compiler_options_set_uint( options, SPVC_COMPILER_OPTION_MSL_PLATFORM, SPVC_MSL_PLATFORM_MACOS );
	// The texture sets become argument buffers so a draw replaces a whole set atomically, the way binding a
	// VkDescriptorSet does. Without that, every set shares one flat per-stage index space that SPIRV-Cross
	// packs densely and *per permutation*, so a resource one program leaves in a slot is read by the next
	// program as something else entirely.
	spvc_compiler_options_set_uint( options, SPVC_COMPILER_OPTION_MSL_ARGUMENT_BUFFERS, SPVC_TRUE );
	// The tier only gates validation inside SPIRV-Cross (unsized descriptor arrays, and writable images on
	// iOS); the struct it emits is identical either way. Report the truth anyway so those checks are real.
	spvc_compiler_options_set_uint( options, SPVC_COMPILER_OPTION_MSL_ARGUMENT_BUFFERS_TIER,
									(unsigned)mtlc_device_argument_buffers_support( rsh.device.mtl.device ) );
	// Bit n enables fragment output location n; a cleared bit turns that [[color(n)]] member into a
	// discarded local. This is how MoltenVK renders into a subpass with fewer attachments than the
	// shader writes, and it is what lets the depth-only variant build as a Metal pipeline.
	if( stage == GLSL_STAGE_FRAGMENT && fragOutputMask != R_MTL_FRAG_OUTPUT_ALL )
		spvc_compiler_options_set_uint( options, SPVC_COMPILER_OPTION_MSL_ENABLE_FRAG_OUTPUT_MASK, fragOutputMask );
	if( spvc_compiler_install_compiler_options( compiler, options ) != SPVC_SUCCESS )
		goto fail;

	// Sets that are pure scratch UBOs stay on the direct path (see R_MTL_SET_IS_ARGUMENT_BUFFER), and so
	// does any set that lost argument-buffer eligibility in reflection. For the rest, pin the buffer index
	// the set's table binds at -- only the container. Pinning the *members* would be actively harmful:
	// in argument-buffer mode CompilerMSL::get_resource_array_size takes an array's length from the
	// registered binding's `count` and ignores the shader's own declaration, so a registration with the
	// wrong count silently collapses lightmapTexture[16] to one element. Member indices are read back
	// after compilation instead.
	for( uint32_t set = 0; set < R_DESCRIPTOR_SET_MAX; set++ ) {
		if( program->programDescriptors[set].mtl.argBufferIndex == R_MTL_SLOT_UNUSED ) {
			if( spvc_compiler_msl_add_discrete_descriptor_set( compiler, set ) != SPVC_SUCCESS )
				goto fail;
			continue;
		}
		spvc_msl_resource_binding_2 ab;
		spvc_msl_resource_binding_init_2( &ab );
		ab.stage = __RP_MTLExecutionModel( stage );
		ab.desc_set = set;
		ab.binding = SPVC_MSL_ARGUMENT_BUFFER_BINDING;
		ab.msl_buffer = program->programDescriptors[set].mtl.argBufferIndex;
		if( spvc_compiler_msl_add_resource_binding_2( compiler, &ab ) != SPVC_SUCCESS )
			goto fail;
	}

	if( spvc_compiler_compile( compiler, &msl ) != SPVC_SUCCESS || !msl )
		goto fail;

	// SPIRV-Cross assigns the MSL resource indices (buffer(n)/texture(n)/sampler(n)) itself. Predicting them
	// with a hand-rolled per-class counter and pinning via add_resource_binding did NOT match the emitted MSL
	// -- e.g. u_BaseSampler landed on a different sampler index -- so the encoder bound resources to the wrong
	// slots and Metal dropped every world draw (validation: "missing sampler binding at index 2 for
	// u_BaseSampler"), leaving the 3D scene black. Read the ground-truth indices back and record them as this
	// stage's slots so RP_BindDescriptorSets binds exactly where MSL expects. Match by (set, binding) decoration
	// rather than name -- SPIRV-Reflect and SPIRV-Cross can spell block names differently. buffer/texture/sampler
	// are independent index spaces; vertex-attribute buffers live at R_MTL_VERTEX_BUFFER_BASE (>= 16) and stay
	// clear of the low buffer indices SPIRV-Cross auto-assigns here.
	{
		// Only resources SPIRV-Cross kept (not dead-stripped) in this stage get a live slot; everything
		// starts UNUSED. Collected locally first so a variant compile can compare without clobbering.
		uint8_t slots[PIPELINE_REFLECTION_HASH_SIZE];
		memset( slots, R_MTL_SLOT_UNUSED, sizeof( slots ) );
		uint8_t pushSlot = R_MTL_SLOT_UNUSED;

		spvc_resources resources = NULL;
		if( spvc_compiler_create_shader_resources( compiler, &resources ) != SPVC_SUCCESS )
			goto fail;
		static const spvc_resource_type kResourceTypes[] = {
			SPVC_RESOURCE_TYPE_UNIFORM_BUFFER,
			SPVC_RESOURCE_TYPE_STORAGE_BUFFER,
			SPVC_RESOURCE_TYPE_SEPARATE_IMAGE,
			SPVC_RESOURCE_TYPE_STORAGE_IMAGE,
			SPVC_RESOURCE_TYPE_SEPARATE_SAMPLERS,
		};
		// The push-constant block is not a descriptor -- it has no set/binding to match on -- but it does
		// take a normal MSL buffer index from the same counter, so it has to be read back too or the bind
		// would guess. SPIRV-Cross reports at most one such block.
		{
			const spvc_reflected_resource *pcList = NULL;
			size_t pcCount = 0;
			if( spvc_resources_get_resource_list_for_type( resources, SPVC_RESOURCE_TYPE_PUSH_CONSTANT, &pcList, &pcCount ) == SPVC_SUCCESS && pcCount > 0 ) {
				const unsigned slot = spvc_compiler_msl_get_automatic_resource_binding( compiler, pcList[0].id );
				if( slot != (unsigned)-1 && slot < R_MTL_SLOT_UNUSED )
					pushSlot = (uint8_t)slot;
			}
		}

		for( size_t t = 0; t < Q_ARRAY_COUNT( kResourceTypes ); t++ ) {
			const spvc_reflected_resource *list = NULL;
			size_t count = 0;
			if( spvc_resources_get_resource_list_for_type( resources, kResourceTypes[t], &list, &count ) != SPVC_SUCCESS )
				continue;
			for( size_t i = 0; i < count; i++ ) {
				const unsigned slot = spvc_compiler_msl_get_automatic_resource_binding( compiler, list[i].id );
				if( slot == (unsigned)-1 )
					continue; // dead-stripped: leave UNUSED so it is not bound
				const unsigned set = spvc_compiler_get_decoration( compiler, list[i].id, SpvDecorationDescriptorSet );
				const unsigned binding = spvc_compiler_get_decoration( compiler, list[i].id, SpvDecorationBinding );
				for( size_t r = 0; r < program->numDescriptorReflections; r++ ) {
					const struct descriptor_reflection_s *reflc = &program->descriptorReflection[r];
					if( reflc->set == set && reflc->baseRegisterIndex == binding ) {
						slots[r] = (uint8_t)slot;
						break;
					}
				}
			}
		}

		if( recordSlots ) {
			for( size_t r = 0; r < program->numDescriptorReflections; r++ )
				program->descriptorReflection[r].mtlSlot[stage] = slots[r];
			program->mtl.pushConstantSlot[stage] = pushSlot;
		} else {
			// Same resource list, same argument-buffer pins, same assignment order -- the indices are
			// expected to match. If SPIRV-Cross ever strips a resource along with the masked output the
			// variant is unusable, because the encoder would bind by the primary's table.
			bool mismatch = ( pushSlot != program->mtl.pushConstantSlot[stage] );
			for( size_t r = 0; r < program->numDescriptorReflections && !mismatch; r++ )
				mismatch = ( slots[r] != program->descriptorReflection[r].mtlSlot[stage] );
			if( mismatch ) {
				Com_Printf( S_COLOR_YELLOW "SPIRV-Cross MSL variant (%s stage %d, output mask 0x%x) lays resources out differently; not used\n",
							program->name ? program->name : "<unnamed>", (int)stage, fragOutputMask );
				spvc_context_destroy( context );
				return NULL;
			}
		}
	}

	// The string belongs to the context, which is destroyed below.
	const size_t len = strlen( msl );
	out = R_Malloc( len + 1 );
	memcpy( out, msl, len + 1 );
	spvc_context_destroy( context );
	return out;

fail: {
	const char *err = spvc_context_get_last_error_string( context );
	Com_Printf( S_COLOR_YELLOW "SPIRV-Cross MSL failed (%s stage %d): %s\n", program->name ? program->name : "<unnamed>", (int)stage, err ? err : "unknown" );
	spvc_context_destroy( context );
	return NULL;
}
}

// Cross-compile each stage and build its MTLLibrary + entry point. SPIRV-Cross always names the entry
// point "main0" (MSL reserves "main"), so that is what is looked up.
// Dump the emitted MSL for one stage next to the preprocessed GLSL the shader compiler already writes
// (logs/shader_debug/), prefixed with the descriptor table this backend derived from it. The whole point
// is that the two are directly comparable: every slot below must appear as the matching
// [[buffer(n)]] / [[texture(n)]] / [[sampler(n)]] (or [[id(n)]], for an argument-buffer set) attribute in
// main0's signature. A mismatch there is the mis-binding this file has to get right.
//
// Names are recovered by re-reflecting the SPIR-V rather than stored on descriptor_reflection_s: this is a
// debug path, the module is still alive (shaderBin is freed only at program teardown), and the alternative
// costs a string per entry across MAX_GLSL_PROGRAMS * PIPELINE_REFLECTION_HASH_SIZE slots.
static const char *__RP_MTLReflectName( const SpvReflectShaderModule *module, unsigned set, unsigned binding )
{
	for( uint32_t i = 0; i < module->descriptor_binding_count; i++ ) {
		const SpvReflectDescriptorBinding *b = &module->descriptor_bindings[i];
		if( b->set == set && b->binding == binding )
			return ( b->name && b->name[0] ) ? b->name : "<anonymous>";
	}
	return "<not-in-stage>";
}

static void __RP_MTLDumpStage( const struct glsl_program_s *program, glsl_program_stage_t stage, const char *msl, const char *suffix )
{
	const struct shader_bin_data_s *bin = &program->shaderBin[stage];
	SpvReflectShaderModule module = { 0 };
	const bool haveModule = bin->bin && bin->size &&
							spvReflectCreateShaderModule( bin->size, bin->bin, &module ) == SPV_REFLECT_RESULT_SUCCESS;

	// qstrcatfmt understands only %c %s %S %i %I %l %u %U %L -- no field widths and no %d/%x -- so every
	// aligned column below is rendered with snprintf first and appended as a plain string.
	char line[256];
	struct QStr out = { 0 };
	struct QStr path = { 0 };

	snprintf( line, sizeof( line ), "// program : %s\n// features: 0x%llx\n// stage    : %s\n", program->name ? program->name : "<unnamed>",
			  (unsigned long long)program->features, RP_GLSLStageToShaderPrefix( stage ) );
	qstrcatfmt( &out, "%s", line );
	if( program->hasPushConstant ) {
		snprintf( line, sizeof( line ), "// push     : size=%u slot=%d\n", (unsigned)program->mtl.pushConstantSize,
				  (int)( program->mtl.pushConstantSlot[stage] == R_MTL_SLOT_UNUSED ? -1 : program->mtl.pushConstantSlot[stage] ) );
		qstrcatfmt( &out, "%s", line );
	}
	qstrcatfmt( &out, "//\n// set bind name                           class    vert  frag  count\n" );
	for( size_t r = 0; r < program->numDescriptorReflections; r++ ) {
		const struct descriptor_reflection_s *refl = &program->descriptorReflection[r];
		const char *cls = ( refl->mtlClass == R_MTL_BIND_TEXTURE ) ? "texture" : ( refl->mtlClass == R_MTL_BIND_SAMPLER ) ? "sampler" : "buffer ";
		const int vs = ( refl->mtlSlot[GLSL_STAGE_VERTEX] == R_MTL_SLOT_UNUSED ) ? -1 : refl->mtlSlot[GLSL_STAGE_VERTEX];
		const int fs = ( refl->mtlSlot[GLSL_STAGE_FRAGMENT] == R_MTL_SLOT_UNUSED ) ? -1 : refl->mtlSlot[GLSL_STAGE_FRAGMENT];
		const char *name = haveModule ? __RP_MTLReflectName( &module, refl->set, refl->baseRegisterIndex ) : "?";
		snprintf( line, sizeof( line ), "//  %u   %2u   %-30s %s  %4d  %4d  %5u\n", (unsigned)refl->set, (unsigned)refl->baseRegisterIndex, name, cls,
				  vs, fs, (unsigned)( refl->dimCount ? refl->dimCount : 1 ) );
		qstrcatfmt( &out, "%s", line );
	}
	qstrcatfmt( &out, "\n%s", msl );

	qstrcatfmt( &path, "logs/shader_debug/%s_%u%s.%s.metal", program->name ? program->name : "unnamed", (unsigned)program->features, suffix,
				RP_GLSLStageToShaderPrefix( stage ) );
	__RP_writeTextToFile( path.buf, out.buf );
	qStrFree( &path );
	qStrFree( &out );
	if( haveModule )
		spvReflectDestroyShaderModule( &module );
}

// Second cross-compile of the fragment stage with every colour output masked off (see
// shader_mtl_stage_s.depthOnlyFn); runs after the primary so the slot table it verifies against exists.
// Failure is deliberately non-fatal: the program still works for every pass with a colour attachment,
// and RP_ResolvePipeline falls back to the full function so Metal reports the depth-only pipeline error
// itself rather than the whole program being dropped.
static void __RP_MTLBuildDepthOnlyVariant( struct glsl_program_s *program )
{
	struct shader_mtl_stage_s *frag = &program->mtlStage[GLSL_STAGE_FRAGMENT];
	// Nothing to strip: the primary already declares no colour output, so it is the depth-only function.
	if( !frag->msl || !strstr( frag->msl, "[[color(" ) )
		return;

	char *msl = __RP_MTLCompileToMSL( program, GLSL_STAGE_FRAGMENT, 0u );
	if( !msl )
		return;
	frag->depthOnlyMsl = msl;

	struct ns_error err = ns_error_from_id( NULL );
	struct mtlc_library lib = mtlc_device_new_library_with_source( rsh.device.mtl.device, ns_string_from_utf8( msl ), mtlc_compile_options_from_id( NULL ), &err );
	if( mtlc_library_is_nil( lib ) ) {
		Com_Printf( S_COLOR_YELLOW "MTLLibrary failed (%s depth-only frag): %s\n", program->name ? program->name : "<unnamed>",
					ns_error_is_nil( err ) ? "unknown" : ns_string_utf8( ns_error_localized_description( err ) ) );
		return;
	}
	struct mtlc_function fn = mtlc_library_new_function( lib, ns_string_from_utf8( "main0" ) );
	if( mtlc_function_is_nil( fn ) ) {
		Com_Printf( S_COLOR_YELLOW "MTLFunction 'main0' missing (%s depth-only frag)\n", program->name ? program->name : "<unnamed>" );
		mtlc_library_release( lib );
		return;
	}
	frag->depthOnlyLib = lib;
	frag->depthOnlyFn = fn;
}

static bool __RP_MTLBuildLibraries( struct glsl_program_s *program, const glsl_program_stage_t *stageList, size_t numStages )
{
	for( size_t i = 0; i < numStages; i++ ) {
		const glsl_program_stage_t stage = stageList[i];
		if( !program->shaderBin[stage].bin )
			continue;

		char *msl = __RP_MTLCompileToMSL( program, stage, R_MTL_FRAG_OUTPUT_ALL );
		if( !msl )
			return false;
		program->mtlStage[stage].msl = msl;

		struct ns_error err = ns_error_from_id( NULL );
		struct mtlc_library lib = mtlc_device_new_library_with_source( rsh.device.mtl.device, ns_string_from_utf8( msl ), mtlc_compile_options_from_id( NULL ), &err );
		if( mtlc_library_is_nil( lib ) ) {
			Com_Printf( S_COLOR_YELLOW "MTLLibrary failed (%s stage %d): %s\n", program->name ? program->name : "<unnamed>", (int)stage,
						ns_error_is_nil( err ) ? "unknown" : ns_string_utf8( ns_error_localized_description( err ) ) );
			return false;
		}
		struct mtlc_function fn = mtlc_library_new_function( lib, ns_string_from_utf8( "main0" ) );
		if( mtlc_function_is_nil( fn ) ) {
			Com_Printf( S_COLOR_YELLOW "MTLFunction 'main0' missing (%s stage %d)\n", program->name ? program->name : "<unnamed>", (int)stage );
			mtlc_library_release( lib );
			return false;
		}
		program->mtlStage[stage].lib = lib;
		program->mtlStage[stage].fn = fn;

		// Vend one argument encoder per set this stage actually uses. Liveness must come from our own
		// reflection: newArgumentEncoderWithBufferIndex: does not return nil for an index the function
		// does not declare, it fails an internal assertion and aborts the process.
		for( uint32_t set = 0; set < R_DESCRIPTOR_SET_MAX; set++ ) {
			struct glsl_program_descriptor_s *programDesc = &program->programDescriptors[set];
			if( programDesc->mtl.argBufferIndex == R_MTL_SLOT_UNUSED )
				continue;
			bool liveInStage = false;
			for( size_t r = 0; r < program->numDescriptorReflections && !liveInStage; r++ )
				liveInStage = ( program->descriptorReflection[r].set == set && program->descriptorReflection[r].mtlSlot[stage] != R_MTL_SLOT_UNUSED );
			if( !liveInStage )
				continue;

			struct mtlc_argument_encoder argEncoder = mtlc_function_new_argument_encoder( fn, programDesc->mtl.argBufferIndex );
			if( mtlc_argument_encoder_is_nil( argEncoder ) ) {
				Com_Printf( S_COLOR_YELLOW "MTLArgumentEncoder missing (%s stage %d set %u)\n", program->name ? program->name : "<unnamed>", (int)stage, set );
				return false;
			}
			program->mtlStage[stage].argEncoder[set] = argEncoder;
			programDesc->mtl.stageLength[stage] = (uint32_t)mtlc_argument_encoder_encoded_length( argEncoder );

			// Each stage's table gets its own aligned region inside one slot, because the two stages are
			// separate cross-compiles and can lay the same set out differently.
			const uint32_t alignment = (uint32_t)mtlc_argument_encoder_alignment( argEncoder );
			programDesc->mtl.stageOffset[stage] = Q_ALIGN_TO( programDesc->mtl.stride, alignment ? alignment : 1 );
			programDesc->mtl.stride = programDesc->mtl.stageOffset[stage] + programDesc->mtl.stageLength[stage];
		}

		if( stage == GLSL_STAGE_FRAGMENT )
			__RP_MTLBuildDepthOnlyVariant( program );
	}
	// A set can survive reflection as argument-buffer eligible and still end up with no encoder, if
	// SPIRV-Cross stripped every one of its resources from both stages. Nothing binds it, and sizing a
	// pool from a zero stride would allocate nothing, so hand it back to the direct path.
	for( uint32_t set = 0; set < R_DESCRIPTOR_SET_MAX; set++ ) {
		if( program->programDescriptors[set].mtl.argBufferIndex != R_MTL_SLOT_UNUSED && program->programDescriptors[set].mtl.stride == 0 )
			program->programDescriptors[set].mtl.argBufferIndex = R_MTL_SLOT_UNUSED;
	}

	// Dumped after the loop, not inside it: the table records a slot per stage, and the second stage's
	// slots are not resolved until its own cross-compile has run.
	for( size_t i = 0; i < numStages; i++ ) {
		const glsl_program_stage_t stage = stageList[i];
		if( program->mtlStage[stage].msl )
			__RP_MTLDumpStage( program, stage, program->mtlStage[stage].msl, "" );
		if( program->mtlStage[stage].depthOnlyMsl )
			__RP_MTLDumpStage( program, stage, program->mtlStage[stage].depthOnlyMsl, "_depthonly" );
	}
	return true;
}

#endif // DEVICE_IMPL_MTL

struct glsl_program_s *RP_RegisterProgram( int type, const char *name, const char *deformsKey, const deformv_t *deforms, int numDeforms, r_glslfeat_t features )
{
	TracyCZoneN( ctx, "RP_RegisterProgram", 1 );
	const uint64_t hashIndex = hash_u64( HASH_INITIAL_VALUE, features ) % GLSL_PROGRAMS_HASH_SIZE;
	struct glsl_program_s *program = r_glslprograms + r_numglslprograms++;
	struct QStr featuresStr = { 0 };
	struct QStr fullName = { 0 };
	qStrAppendSlice( &fullName, qCToStrRef( name ) );
	for( feature_iter_t iter = __R_NextFeature( (feature_iter_t){ .it = glsl_programtypes_features[type], .bits = features } ); __R_IsValidFeatureIter( &iter ); iter = __R_NextFeature( iter ) ) {
		qstrcatfmt( &featuresStr, "%s\n", iter.it->define );
		if( !qStrEmpty( fullName ) ) {
			qStrAppendSlice( &fullName, qCToStrRef( iter.it->suffix ) );
		}
	}
	ri.Com_Printf( "Loading Shader: %.*s", fullName.len, fullName.buf );

	// Named up front so every diagnostic and MSL dump the build path emits carries the program's name
	// and feature bits rather than "<unnamed>" / 0 (which also made every dump overwrite the same file).
	// A failed slot is torn down by RP_Shutdown exactly like a successful one, so nothing leaks.
	qStrSetNullTerm( &fullName );
	program->name = R_CopyString( fullName.buf );
	program->type = type;
	program->features = features;

	bool error = false;
	struct {
		glsl_program_stage_t stage;
		struct QStr source;
	} stages[] = {
		{ .stage = GLSL_STAGE_VERTEX, .source = { 0 } },
		{ .stage = GLSL_STAGE_FRAGMENT, .source = { 0 } },
	};
	{
		struct QStr filePath = { 0 };
		for( size_t i = 0; i < Q_ARRAY_COUNT( stages ); i++ ) {
			qStrAppendSlice( &stages[i].source, qCToStrRef( "#version 440 \n" ) );
			switch( stages[i].stage ) {
				case GLSL_STAGE_VERTEX:
					qStrAppendSlice( &stages[i].source, qCToStrRef( "#define VERTEX_SHADER\n" ) );
					break;
				case GLSL_STAGE_FRAGMENT:
					qStrAppendSlice( &stages[i].source, qCToStrRef( "#define FRAGMENT_SHADER\n" ) );
					break;
				default:
					assert( 0 );
					break;
			}
			qStrAppendSlice( &stages[i].source, qToStrRef( featuresStr ) );
			qStrAppendSlice( &stages[i].source, qCToStrRef( QF_BUILTIN_GLSL_CONSTANTS ) );
			qStrAppendSlice( &stages[i].source, qCToStrRef( QF_GLSL_MATH ) );
			if( stages[i].stage == GLSL_STAGE_VERTEX ) {
				__appendGLSLDeformv( &stages[i].source, deforms, numDeforms );
			}

			qStrClear( &filePath );
			qstrcatfmt( &filePath, "glsl_nri/%s.%s.glsl", name, RP_GLSLStageToShaderPrefix( stages[i].stage ) );
			qStrSetNullTerm( &filePath );
			error = RF_AppendShaderFromFile( &stages[i].source, filePath.buf, type, features );
			if( error ) {
				break;
			}
		}
		qStrFree( &filePath );
	}
	qStrFree( &featuresStr );

	program->hasPushConstant = false;
	program->vertexInputMask = 0;

	for( size_t i = 0; i < Q_ARRAY_COUNT( stages ); i++ ) {
		qStrSetNullTerm( &stages[i].source );
		const glslang_input_t input = { .language = GLSLANG_SOURCE_GLSL,
										.stage = __RP_GLStageToSlang( stages[i].stage ),
										.client = GLSLANG_CLIENT_VULKAN,
										.client_version = GLSLANG_TARGET_VULKAN_1_2,
										.target_language = GLSLANG_TARGET_SPV,
										.target_language_version = GLSLANG_TARGET_SPV_1_5,
										.code = stages[i].source.buf,
										.default_version = 450,
										.default_profile = GLSLANG_CORE_PROFILE,
										.force_default_version_and_profile = false,
										.forward_compatible = false,
										.messages = GLSLANG_MSG_DEFAULT_BIT,
										.resource = glslang_default_resource() };
		glslang_shader_t *shader = glslang_shader_create( &input );
		glslang_program_t *glslang_program = NULL;
		if( !glslang_shader_preprocess( shader, &input ) ) {
			struct QStr errFilePath = { 0 };
			qstrcatfmt( &errFilePath, "logs/shader_err/%s.%s.glsl", name, RP_GLSLStageToShaderPrefix( stages[i].stage ) );
			const char *infoLog = glslang_shader_get_info_log( shader );
			const char *debugLogs = glslang_shader_get_info_debug_log( shader );
			Com_Printf( S_COLOR_YELLOW "GLSL preprocess failed %.*s\n", fullName.len, fullName.buf );
			Com_Printf( S_COLOR_YELLOW "%s\n", infoLog );
			Com_Printf( S_COLOR_YELLOW "%s\n", debugLogs );
			Com_Printf( S_COLOR_YELLOW "dump shader: %.*s\n", errFilePath.len, errFilePath.buf );

			struct QStr shaderErr = { 0 };
			qstrcatfmt( &shaderErr, "%s\n", input.code );
			qstrcatfmt( &shaderErr, "----------- Preprocessing Failed -----------\n" );
			qstrcatfmt( &shaderErr, "%s\n", infoLog );
			qstrcatfmt( &shaderErr, "%s\n", debugLogs );
			__RP_writeTextToFile( errFilePath.buf, shaderErr.buf );
			assert( false && "failed to preprocess shader" );

			qStrFree( &shaderErr );
			qStrFree( &errFilePath );

			error = true;
			goto shader_done;
		}

		if( !glslang_shader_parse( shader, &input ) ) {
			struct QStr errFilePath = { 0 };
			qstrcatfmt( &errFilePath, "logs/shader_err/%s.%s.glsl", name, RP_GLSLStageToShaderPrefix( stages[i].stage ) );
			const char *infoLog = glslang_shader_get_info_log( shader );
			const char *debugLogs = glslang_shader_get_info_debug_log( shader );

			Com_Printf( S_COLOR_YELLOW "GLSL parsing failed %.*s\n", fullName.len, fullName.buf );
			Com_Printf( S_COLOR_YELLOW "%s\n", infoLog );
			Com_Printf( S_COLOR_YELLOW "%s\n", debugLogs );
			Com_Printf( S_COLOR_YELLOW "dump shader: %.*s\n", errFilePath.len, errFilePath.buf );

			struct QStr shaderErr = { 0 };
			qstrcatfmt( &shaderErr, "%s\n", input.code );
			qstrcatfmt( &shaderErr, "----------- Parsing Failed -----------\n" );
			qstrcatfmt( &shaderErr, "%s\n", infoLog );
			qstrcatfmt( &shaderErr, "%s\n", debugLogs );
			__RP_writeTextToFile( errFilePath.buf, shaderErr.buf );
			assert( false && "failed to parse shader" );
			qStrFree( &shaderErr );
			qStrFree( &errFilePath );

			error = true;
			goto shader_done;
		} else {
			struct QStr shaderDebugPath = { 0 };
			qstrcatfmt( &shaderDebugPath, "logs/shader_debug/%S_%u.%s.glsl", qToStrRef( fullName ), features, RP_GLSLStageToShaderPrefix( stages[i].stage ) );
			__RP_writeTextToFile( shaderDebugPath.buf, glslang_shader_get_preprocessed_code( shader ) );
			qStrFree( &shaderDebugPath );
		}

		glslang_program = glslang_program_create();
		glslang_program_add_shader( glslang_program, shader );

		if( !glslang_program_link( glslang_program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT ) ) {
			struct QStr errFilePath = { 0 };
			qstrcatfmt( &errFilePath, "logs/shader_err/%s.%s.glsl", name, RP_GLSLStageToShaderPrefix( stages[i].stage ) );

			const char *infoLogs = glslang_program_get_info_log( glslang_program );
			const char *debugLogs = glslang_program_get_info_debug_log( glslang_program );

			Com_Printf( S_COLOR_YELLOW "GLSL linking failed %.*s\n", fullName.len, fullName.buf );
			Com_Printf( S_COLOR_YELLOW "%s\n", infoLogs );
			Com_Printf( S_COLOR_YELLOW "%s\n", debugLogs );
			Com_Printf( S_COLOR_YELLOW "dump shader: %.*s\n", errFilePath.len, errFilePath.buf );

			struct QStr shaderErr = { 0 };
			qstrcatfmt( &shaderErr, "%s\n", input.code );
			qstrcatfmt( &shaderErr, "----------- Linking Failed -----------\n" );
			qstrcatfmt( &shaderErr, "%s\n", infoLogs );
			qstrcatfmt( &shaderErr, "%s\n", debugLogs );
			__RP_writeTextToFile( errFilePath.buf, shaderErr.buf );
			qStrFree( &shaderErr );
			qStrFree( &errFilePath );

			assert( false && "failed to link shader" );
			error = true;
			goto shader_done;
		}

		const char *spirv_messages = glslang_program_SPIRV_get_messages( glslang_program );
		if( spirv_messages ) {
			Com_Printf( S_COLOR_BLUE "(%s) %s\b", name, spirv_messages );
		}

		// TODO: spv needs to be optimized for release
		glslang_spv_options_t spvOptions = { 0 };
		spvOptions.disable_optimizer = false;
		spvOptions.optimize_size = true;
		spvOptions.validate = true;
		glslang_program_SPIRV_generate_with_options( glslang_program, __RP_GLStageToSlang( stages[i].stage ), &spvOptions );
		size_t binSize = glslang_program_SPIRV_get_size( glslang_program ) * sizeof( uint32_t );
		struct shader_bin_data_s *binData = &program->shaderBin[stages[i].stage];
		binData->bin = R_Malloc( binSize );
		binData->size = binSize;
		glslang_program_SPIRV_get( glslang_program, (uint32_t *)binData->bin );
		binData->stage = stages[i].stage;

	shader_done:
		glslang_shader_delete( shader );
		if( glslang_program ) {
			glslang_program_delete( glslang_program );
		}
	}
	SpvReflectDescriptorSet **reflectionDescSets = NULL;

#if ( DEVICE_IMPL_MTL )
	// Metal counterpart to the Vulkan reflection block below. It fills the same backend-neutral fields
	// (push constants, vertexInputMask, per-set counts, descriptorReflection) but skips the descriptor
	// set / pipeline layout objects, which Metal has no equivalent of, then cross-compiles to MSL.
	if( !error ) {
		if( !__RP_MTLReflectProgram( program, (const glsl_program_stage_t[]){ GLSL_STAGE_VERTEX, GLSL_STAGE_FRAGMENT }, 2 ) )
			error = true;
		else if( !__RP_MTLBuildLibraries( program, (const glsl_program_stage_t[]){ GLSL_STAGE_VERTEX, GLSL_STAGE_FRAGMENT }, 2 ) )
			error = true;
	}
#endif

#if ( DEVICE_IMPL_VULKAN )
	{
		SpvReflectBlockVariable *reflectionBlockVariables[1] = { 0 };
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		VkDescriptorSetLayoutBinding *descriptorSetLayoutBindings[R_DESCRIPTOR_SET_MAX] = { 0 };
		VkDescriptorBindingFlags *descriptorBindingFlags[R_DESCRIPTOR_SET_MAX] = { 0 };

		VkDescriptorSetLayout setLayouts[R_DESCRIPTOR_SET_MAX] = { 0 };
		VkPushConstantRange pushConstantRange = { 0 };
		for( size_t i = 0; i < Q_ARRAY_COUNT( stages ); i++ ) {
			SpvReflectShaderModule module = { 0 };
			SpvReflectResult result = spvReflectCreateShaderModule( program->shaderBin[stages[i].stage].size, program->shaderBin[stages[i].stage].bin, &module );
			assert( result == SPV_REFLECT_RESULT_SUCCESS );
			{
				uint32_t pushConstantCount = 0;
				result = spvReflectEnumeratePushConstantBlocks( &module, &pushConstantCount, NULL );
				assert( result == SPV_REFLECT_RESULT_SUCCESS );
				program->hasPushConstant |= ( pushConstantCount > 0 );
				if( pushConstantCount > 0 ) {
					if( pushConstantCount > 1 ) {
						Com_Printf( S_COLOR_YELLOW "Push constant count is greater than 1, only supporting 1 push constant\n" );
						error = true;
						break;
					}
					pipelineLayoutCreateInfo.pushConstantRangeCount = 1;

					result = spvReflectEnumeratePushConstantBlocks( &module, &pushConstantCount, reflectionBlockVariables );
					assert( result == SPV_REFLECT_RESULT_SUCCESS );
					pushConstantRange.size = reflectionBlockVariables[0]->size;
					program->vk.pushConstant.size = reflectionBlockVariables[0]->size;
					switch( stages[i].stage ) {
						case GLSL_STAGE_VERTEX:
							pushConstantRange.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
							program->vk.pushConstant.shaderStageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
							break;
						case GLSL_STAGE_FRAGMENT:
							pushConstantRange.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
							program->vk.pushConstant.shaderStageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
							break;
						default:
							assert( false );
							break;
					}
				}

				if( stages[i].stage == GLSL_STAGE_VERTEX ) {
					for( size_t i = 0; i < module.input_variable_count; i++ ) {
						program->vertexInputMask |= ( 1 << module.input_variables[i]->location );
					}
				}

				uint32_t reflectionDescriptorCount = 0;
				result = spvReflectEnumerateDescriptorSets( &module, &reflectionDescriptorCount, NULL );
				assert( result == SPV_REFLECT_RESULT_SUCCESS );

				arrsetlen( reflectionDescSets, reflectionDescriptorCount );
				result = spvReflectEnumerateDescriptorSets( &module, &reflectionDescriptorCount, reflectionDescSets );
				assert( result == SPV_REFLECT_RESULT_SUCCESS );
				for( size_t i_set = 0; i_set < reflectionDescriptorCount; i_set++ ) {
					const SpvReflectDescriptorSet *reflection = reflectionDescSets[i_set];
					assert( reflection->set < Q_ARRAY_COUNT( program->programDescriptors ) );
					struct glsl_program_descriptor_s *programDesc = &program->programDescriptors[reflection->set];
					programDesc->alloc.descriptorAllocator = _vk__descriptorSetAlloc;
					programDesc->alloc.framesInFlight = NUMBER_FRAMES_FLIGHT;
					for( size_t i_binding = 0; i_binding < reflection->binding_count; i_binding++ ) {
						const SpvReflectDescriptorBinding *reflectionBinding = reflection->bindings[i_binding];
						assert( reflection->set < R_DESCRIPTOR_SET_MAX );
						assert( reflectionBinding->array.dims_count <= 1 ); // not going to handle multi-dim arrays
						struct descriptor_reflection_s reflc = { 0 };
						reflc.hash = Create_DescriptorHandle( reflectionBinding->name ).hash;
						reflc.set = reflectionBinding->set;
						reflc.baseRegisterIndex = reflectionBinding->binding;
						reflc.isArray = reflectionBinding->count > 1;
						reflc.dimCount = max( 1, reflectionBinding->count );

						VkDescriptorSetLayoutBinding *layoutBinding = NULL;
						VkDescriptorBindingFlags *bindingFlags = NULL;
						for( size_t i = 0; i < arrlen( descriptorSetLayoutBindings[reflection->set] ); i++ ) {
							if( descriptorSetLayoutBindings[reflection->set][i].binding == reflectionBinding->binding ) {
								layoutBinding = descriptorSetLayoutBindings[reflection->set] + i;
								bindingFlags = descriptorBindingFlags[reflection->set] + i;
							}
						}

						if( !layoutBinding ) {
							VkDescriptorSetLayoutBinding bindings = { 0 };
							VkDescriptorBindingFlags flags = 0;
							arrpush( descriptorSetLayoutBindings[reflection->set], bindings );
							arrpush( descriptorBindingFlags[reflection->set], flags );
							layoutBinding = descriptorSetLayoutBindings[reflection->set] + arrlen( descriptorSetLayoutBindings[reflection->set] ) - 1;
							bindingFlags = descriptorBindingFlags[reflection->set] + arrlen( descriptorBindingFlags[reflection->set] ) - 1;
						}

						if( reflc.isArray ) {
							( *bindingFlags ) = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
						}

						const uint32_t bindingCount = max( reflectionBinding->count, 1 );
						layoutBinding->binding = reflectionBinding->binding;
						layoutBinding->descriptorCount = bindingCount;
						switch( stages[i].stage ) {
							case GLSL_STAGE_VERTEX:
								layoutBinding->stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
								break;
							case GLSL_STAGE_FRAGMENT:
								layoutBinding->stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
								break;
							default:
								assert( false );
								break;
						}
						switch( reflectionBinding->descriptor_type ) {
							case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
								layoutBinding->descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
								programDesc->samplerMaxNum += bindingCount;
								break;
							case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
								layoutBinding->descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
								programDesc->textureMaxNum += bindingCount;
								break;
							case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
								layoutBinding->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
								programDesc->bufferMaxNum += bindingCount;
								break;
							case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
								layoutBinding->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
								programDesc->storageTextureMaxNum += bindingCount;
								break;
							case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
								layoutBinding->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
								programDesc->storageBufferMaxNum += bindingCount;
								break;
							case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
								layoutBinding->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
								programDesc->constantBufferMaxNum += bindingCount;
								break;
							case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
							case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
								layoutBinding->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
								programDesc->structuredBufferMaxNum += bindingCount;
								break;
							case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
							case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
							case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
							case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
								assert( false );
								break;
						}
						assert( program->numDescriptorReflections < PIPELINE_REFLECTION_HASH_SIZE );
						program->descriptorReflection[program->numDescriptorReflections++] = reflc;
					}
				}
			}
		}

		uint32_t numLayoutCount = 0;
		for( size_t bindingIdx = 0; bindingIdx < R_DESCRIPTOR_SET_MAX; bindingIdx++ ) {
			if( descriptorSetLayoutBindings[bindingIdx] ) {
				numLayoutCount = bindingIdx + 1;
			}
		}

		for( size_t bindingIdx = 0; bindingIdx < numLayoutCount; bindingIdx++ ) {
			if( descriptorSetLayoutBindings[bindingIdx] ) {
				VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
				bindingFlagsInfo.bindingCount = arrlen( descriptorBindingFlags[bindingIdx] );
				bindingFlagsInfo.pBindingFlags = descriptorBindingFlags[bindingIdx];

				VkDescriptorSetLayoutCreateInfo createSetLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
				createSetLayoutInfo.bindingCount = arrlen( descriptorSetLayoutBindings[bindingIdx] );
				createSetLayoutInfo.pBindings = descriptorSetLayoutBindings[bindingIdx];
				R_VK_ADD_STRUCT( &createSetLayoutInfo, &bindingFlagsInfo );

				VK_WrapResult( vkCreateDescriptorSetLayout( rsh.device.vk.device, &createSetLayoutInfo, NULL, setLayouts + bindingIdx ) );
				arrfree( descriptorSetLayoutBindings[bindingIdx] );
				arrfree( descriptorBindingFlags[bindingIdx] );
			} else {
				VkDescriptorSetLayoutCreateInfo createSetLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
				VK_WrapResult( vkCreateDescriptorSetLayout( rsh.device.vk.device, &createSetLayoutInfo, NULL, setLayouts + bindingIdx ) );
			}
			program->programDescriptors[bindingIdx].vk.setLayout = setLayouts[bindingIdx];
		}
		pipelineLayoutCreateInfo.pSetLayouts = setLayouts;
		pipelineLayoutCreateInfo.setLayoutCount = numLayoutCount;
		if( pushConstantRange.stageFlags > 0 )
			pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
		VK_WrapResult( vkCreatePipelineLayout( rsh.device.vk.device, &pipelineLayoutCreateInfo, NULL, &program->vk.pipelineLayout ) );
	}
#endif
	arrfree( reflectionDescSets );

	program->deformsKey = R_CopyString( deformsKey ? deformsKey : "" );

	//	if(rootConstantDesc.size > 0) {
	//		pipelineLayoutDesc.rootConstantNum = 1;
	//		pipelineLayoutDesc.rootConstants = &rootConstantDesc;
	//	}
	//
	// pipelineLayoutDesc.shaderStages = NriStageBits_GRAPHICS_SHADERS;
	// pipelineLayoutDesc.descriptorSetNum = program->numSets;
	// for( size_t i = 0; i < program->numSets; i++ ) {
	// 	//			struct ProgramDescriptorInfo* info = &program->descriptorInfo[i];
	// 	//			info->setIndex = pipelineLayoutDesc.descriptorSetNum;
	// 	descriptorSetDesc[i].registerSpace = program->descriptorSetInfo[i].registerSpace;
	// 	const uint32_t setIndex = program->descriptorSetInfo[i].setIndex;
	// 	descriptorSetDesc[i].rangeNum = arrlen( descRangeDescs[setIndex] );
	// 	descriptorSetDesc[i].ranges = descRangeDescs[setIndex];
	// 	//	ri.Com_Printf( "Using Descriptor Set[%d]", i );
	// 	// for( size_t l = 0; l < arrlen( descRangeDescs[i] ); l++ ) {
	// 	// 	ri.Com_Printf( "[%lu]    Register - %lu ", l, descRangeDescs[i][l].baseRegisterIndex);
	// 	// 	ri.Com_Printf( "[%lu]      descriptorNum: %lu ", l, descRangeDescs[i][l].descriptorNum);
	// 	// 	ri.Com_Printf( "[%lu]      DescriptorType: %s ", l, NriDescriptorTypeToString[descRangeDescs[i][l].descriptorType]);
	// 	// 	ri.Com_Printf( "[%lu]      Vertex: %s", l, (descRangeDescs[i][l].shaderStages & NriStageBits_VERTEX_SHADER) ? "true" : "false");
	// 	// 	ri.Com_Printf( "[%lu]      Fragment: %s ", l, (descRangeDescs[i][l].shaderStages & NriStageBits_FRAGMENT_SHADER) ? "true": "false");
	// 	// }
	// }
	// pipelineLayoutDesc.ignoreGlobalSPIRVOffsets = true;
	// pipelineLayoutDesc.descriptorSets = descriptorSetDesc;
	// NRI_ABORT_ON_FAILURE(rsh.nri.coreI.CreatePipelineLayout(rsh.nri.device, &pipelineLayoutDesc, &program->layout));

	// for( size_t i = 0; i < DESCRIPTOR_SET_MAX; i++ ) {
	//	arrfree( descRangeDescs[i] );
	// }

	qStrFree( &fullName );
	for( size_t i = 0; i < Q_ARRAY_COUNT( stages ); i++ ) {
		qStrFree( &stages[i].source );
	}

	if( !program->hash_next ) {
		program->hash_next = r_glslprograms_hash[type][hashIndex];
		r_glslprograms_hash[type][hashIndex] = program;
	}

	TracyCZoneEnd( ctx );
	return program;
}
/*
 * RP_Shutdown
 */
void RP_Shutdown( void )
{
	unsigned int i;
	struct glsl_program_s *program;
	TracyCZoneN( ctx, "RP_Shutdown", 1 );

	for( i = 0, program = r_glslprograms; i < r_numglslprograms; i++, program++ ) {
		RF_DeleteProgram( program );
	}

#if ( DEVICE_IMPL_MTL )
	// The depth-stencil cache is process-wide rather than per-program, so it is released here.
	__RP_MTLReleaseDepthStates();
#endif

	Trie_Destroy( glsl_cache_trie );
	glsl_cache_trie = NULL;

	r_numglslprograms = 0;
	TracyCZoneEnd( ctx );
}
