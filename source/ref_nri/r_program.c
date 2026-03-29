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

#include "spirv_reflect.h"
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Include/glslang_c_shader_types.h>
#include <glslang/Public/resource_limits_c.h>

#include "r_vattribs.h"

#include "ri_conversion.h"
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
				qstrcatprintf( str, "#define DEFORMV_WAVE_CONSTANT vec4(%f,%f,%f,%f) \n", deformv->func.args[0], deformv->func.args[1], deformv->func.args[2], deformv->func.args[3] );
				break;
			}
			case DEFORMV_MOVE: {
				if( funcType <= SHADER_FUNC_NONE || funcType > numSupportedFuncs || !funcs[funcType] )
					break;

				qStrAppendSlice( str, qCToStrRef( "#define DEFORMV_MOVE 1 \n" ) );
				qstrcatfmt( str, "#define DEFORMV_MOVE_FUNC %s \n", funcs[funcType] );
				qstrcatprintf( str, "#define DEFORMV_MOVE_CONSTANT vec4(%f,%f,%f,%f) \n", deformv->func.args[0], deformv->func.args[1], deformv->func.args[2], deformv->func.args[3] );

				break;
			}
			case DEFORMV_BULGE:
				qStrAppendSlice( str, qCToStrRef( "#define DEFORMV_BULGE 1 \n" ) );
				qstrcatprintf( str, "#define DEFORMV_BULGE_CONSTANT vec4(%f,%f,%f,%f) \n", deformv->func.args[0], deformv->func.args[1], deformv->func.args[2], deformv->func.args[3] );
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
	for( size_t i = 0; i < GLSL_STAGE_MAX; i++ ) {
		if( program->shaderBin[i].bin )
			R_Free( program->shaderBin[i].bin );
	}

	hash_next = program->hash_next;
	memset( program, 0, sizeof( struct glsl_program_s ) );
	program->hash_next = hash_next;
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

static int __VK_SortVkVertexInputAttributeDescription( const void *a1, const void *a2 )
{
	const struct VkVertexInputAttributeDescription *at1 = a1;
	const struct VkVertexInputAttributeDescription *at2 = a2;
	return at1->location > at2->location;
}

void RP_BindPipeline( struct FrameState_s *cmd, struct pipeline_hash_s *pipeline )
{
#if ( DEVICE_IMPL_VULKAN )
	vkCmdBindPipeline( cmd->handle.vk.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vk.handle );
#endif
}

struct pipeline_hash_s *RP_ResolvePipeline( struct glsl_program_s *program, struct pipeline_desc_s *cmd )
{
	enum RICullMode_e cullMode = cmd->cullMode;
	if( cmd->flippedViewport ) {
		cullMode = ~cullMode;
	}
	VkVertexInputAttributeDescription vertextbindingDesc[MAX_ATTRIBUTES];
	VkVertexInputBindingDescription vertexInputStreamsDesc[MAX_STREAMS];
	VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VkFormat colorAttachmentsVK[MAX_COLOR_ATTACHMENTS];

	const bool useDepthBias = ( cmd->depthBiasConstant != 0 || cmd->depthBiasSlope != 0 );

	//	uint32_t attribFlags = 0;
	assert( cmd->numStreams < MAX_ATTRIBUTES );
	assert( cmd->numStreams < MAX_STREAMS );
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
	} encode = {
		.depthBiasConstant = useDepthBias ? cmd->depthBiasConstant : 0,
		.depthBiasClamp = useDepthBias ? cmd->depthBiasClamp : 0,
		.depthBiasSlope = useDepthBias ? cmd->depthBiasSlope : 0,
		.cullMode = cullMode,
		.colorBlendEnabled = cmd->colorBlendEnabled,
		.depthWrite = cmd->depthWrite,
		.colorWriteMask = cmd->colorBlendEnabled ? cmd->colorWriteMask : 0,
		.colorSrcFactor = cmd->colorBlendEnabled ? cmd->colorSrcFactor : 0,
		.colorDstFactor = cmd->colorBlendEnabled ? cmd->colorDstFactor : 0,
		.topology = cmd->topology,
		.compareFunc = cmd->compareFunc,
	};
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

	struct pipeline_hash_s *pipeline = __resolvePipeline( program, hash );
	assert( pipeline );
	if( pipeline->vk.handle ) {
		return pipeline; // pipeline is present in slot
	}
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
			colorAttachmentDesc[i].colorWriteMask = ri_vk_RIColorWriteMaskToVK( cmd->colorWriteMask );
		}

		VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		if( cmd->numColorsAttachments > 0 ) {
			colorBlendState.attachmentCount = cmd->numColorsAttachments;
			colorBlendState.pAttachments = colorAttachmentDesc;
			pipelineCreateInfo.pColorBlendState = &colorBlendState;
		}

		VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewportState.viewportCount = Q_MAX( 1, cmd->numColorsAttachments );
		viewportState.scissorCount = Q_MAX( 1, cmd->numColorsAttachments );
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
#if ( DEVICE_IMPL_VULKAN )
	{
		assert( len <= program->vk.pushConstant.size );
		vkCmdPushConstants( cmd->handle.vk.cmd, program->vk.pipelineLayout, program->vk.pushConstant.shaderStageFlags, 0, program->vk.pushConstant.size, data );
	}
#endif
}

void RP_BindDescriptorSets( struct RIDevice_s *device, struct FrameState_s *cmd, struct glsl_program_s *program, struct glsl_descriptor_binding_s *bindings, size_t numDescriptorData )
{
#if ( DEVICE_IMPL_VULKAN )
	{
		size_t numWrites = 0;
		VkWriteDescriptorSet descriptorWrite[32]; // write 32 descriptors at once
		for( uint32_t setIndex = 0; setIndex < R_DESCRIPTOR_SET_MAX; setIndex++ ) {
			hash_t hash = HASH_INITIAL_VALUE;
			for( size_t i = 0; i < numDescriptorData; i++ ) {
				const struct descriptor_reflection_s *refl = __ReflectDescriptorSet( program, &bindings[i].handle );
				if( !refl || setIndex != refl->set || RI_IsEmptyDescriptor( &bindings[i].descriptor ) )
					continue;
				hash = hash_u64( hash, refl->hash );
				hash = hash_u64( hash, bindings[i].descriptor.cookie );
			}
			if( hash == HASH_INITIAL_VALUE )
				continue;
			struct glsl_program_descriptor_s *info = &program->programDescriptors[setIndex];
			struct descriptor_set_result_s result = ResolveDescriptorSet( &rsh.device, &info->alloc, rsh.frameSetCount, hash );
			if( !result.found ) {
				for( size_t i = 0; i < numDescriptorData; i++ ) {
					const struct descriptor_reflection_s *refl = __ReflectDescriptorSet( program, &bindings[i].handle );
					if( !refl || setIndex != refl->set || RI_IsEmptyDescriptor( &bindings[i].descriptor ) )
						continue;

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
					vkDesc->descriptorType = bindings[i].descriptor.vk.type;
					switch( bindings[i].descriptor.vk.type ) {
						case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
						case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
							vkDesc->pBufferInfo = &bindings[i].descriptor.vk.buffer;
							break;
						case VK_DESCRIPTOR_TYPE_SAMPLER:
						case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
						case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
							vkDesc->pImageInfo = &bindings[i].descriptor.vk.image;
							break;
						default:
							assert( false ); // this is bad
							break;
					}

					if( numWrites >= Q_ARRAY_COUNT( descriptorWrite ) ) {
						vkUpdateDescriptorSets( device->vk.device, numWrites, descriptorWrite, 0, NULL );
						numWrites = 0;
					}
				}
			}
			VkDescriptorSet vkDescriptorSet = result.set->vk.handle;
			vkCmdBindDescriptorSets( cmd->handle.vk.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, program->vk.pipelineLayout, setIndex, 1, &vkDescriptorSet, 0, NULL );
		}
		if( numWrites > 0 ) {
			vkUpdateDescriptorSets( device->vk.device, numWrites, descriptorWrite, 0, NULL );
		}
	}
#endif
}

struct glsl_program_s *RP_ResolveProgram( int type, const char *name, const char *deformsKey, const deformv_t *deforms, int numDeforms, r_glslfeat_t features )
{
	assert( type > GLSL_PROGRAM_TYPE_NONE && type < GLSL_PROGRAM_TYPE_MAXTYPE );
	assert( !deforms || deformsKey );

	if( !deforms )
		deformsKey = "";

	const uint64_t hashIndex = hash_u64( HASH_INITIAL_VALUE, features ) % GLSL_PROGRAMS_HASH_SIZE;
	for( struct glsl_program_s *program = r_glslprograms_hash[type][hashIndex]; program; program = program->hash_next ) {
		if( ( program->features == features ) && strcmp( program->deformsKey, deformsKey ) == 0 ) {
			return program;
		}
	}

	if( r_numglslprograms == MAX_GLSL_PROGRAMS ) {
		Com_Printf( S_COLOR_YELLOW "RP_RegisterProgram: GLSL programs limit exceeded\n" );
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
			return 0;
		}
	}

	return RP_RegisterProgram( type, name, deformsKey, deforms, numDeforms, features );
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
	assert( descriptorPoolLen < Q_ARRAY_COUNT( descriptorPoolSize ) );
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

struct glsl_program_s *RP_RegisterProgram( int type, const char *name, const char *deformsKey, const deformv_t *deforms, int numDeforms, r_glslfeat_t features )
{
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

	program->type = type;
	program->features = features;
	qStrSetNullTerm( &fullName );
	program->name = R_CopyString( fullName.buf );
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

	return program;
}
/*
 * RP_Shutdown
 */
void RP_Shutdown( void )
{
	unsigned int i;
	struct glsl_program_s *program;

	for( i = 0, program = r_glslprograms; i < r_numglslprograms; i++, program++ ) {
		RF_DeleteProgram( program );
	}

	Trie_Destroy( glsl_cache_trie );
	glsl_cache_trie = NULL;

	r_numglslprograms = 0;
}
