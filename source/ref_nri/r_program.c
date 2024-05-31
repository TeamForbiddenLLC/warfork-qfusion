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

#include "r_descriptor_pool.h"
#include "r_local.h"
#include "../qalgo/q_trie.h"
#include "./r_hasher.h"

#include "glslang/Include/glslang_c_interface.h"
#include "glslang/Public/resource_limits_c.h"
#include <glslang/Include/glslang_c_shader_types.h>
#include "spirv_reflect.h"

#include "../gameshared/q_sds.h"
#include "r_vattribs.h"
#include "r_nri.h"
#include "stb_ds.h"

#include "../../gameshared/q_arch.h"

#define MAX_GLSL_PROGRAMS			1024
#define GLSL_PROGRAMS_HASH_SIZE		256

#ifdef GL_ES_VERSION_2_0
#define GLSL_DEFAULT_CACHE_FILE_NAME	"glsl/glsles.cache.default"
#else
#define GLSL_DEFAULT_CACHE_FILE_NAME	"glsl/glsl.cache.default"
#endif

#define GLSL_CACHE_FILE_NAME			"cache/glsl.cache"
#define GLSL_BINARY_CACHE_FILE_NAME		"cache/glsl.cache.bin"


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

static int r_glslbincache_storemode;

static void RP_GetUniformLocations( struct glsl_program_s *program );
static void RP_BindAttrbibutesLocations( struct glsl_program_s *program );
static void *RP_GetProgramBinary( int elem, int *format, unsigned *length );

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
	int program = RP_RegisterProgram( GLSL_PROGRAM_TYPE_MATERIAL, DEFAULT_GLSL_MATERIAL_PROGRAM, NULL, NULL, 0, GLSL_SHADER_COMMON_BONE_TRANSFORMS1 );
}

typedef struct {
	const glsl_feature_t *it;
	r_glslfeat_t bits;
} feature_iter_t;

static bool __R_IsValidFeatureIter( feature_iter_t *iter )
{
	return iter->it != NULL && iter->bits > 0;
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

static sds R_AppendGLSLDeformv( sds str, const deformv_t *deformv, int numDeforms )
{
	static const char *const funcs[] = { NULL, "WAVE_SIN", "WAVE_TRIANGLE", "WAVE_SQUARE", "WAVE_SAWTOOTH", "WAVE_INVERSESAWTOOTH", NULL };
	static const int numSupportedFuncs = sizeof( funcs ) / sizeof( funcs[0] ) - 1;
	if( !numDeforms ) {
		return str;
	}
 // str = sdscatfmt( str,
 // 				 "#define QF_APPLY_DEFORMVERTS\n"
 // 				 "#if defined(APPLY_AUTOSPRITE) || defined(APPLY_AUTOSPRITE2)\n"
 // 				 "qf_attribute vec4 a_SpritePoint;\n"
 // 				 "#else\n"
 // 				 "#define a_SpritePoint vec4(0.0)\n"
 // 				 "#endif\n"
 // 				 "\n"
 // 				 "#if defined(APPLY_AUTOSPRITE2)\n"
 // 				 "qf_attribute vec4 a_SpriteRightUpAxis;\n"
 // 				 "#else\n"
 // 				 "#define a_SpriteRightUpAxis vec4(0.0)\n"
 // 				 "#endif\n"
 // 				 "\n"
 // 				 "void QF_DeformVerts(inout vec4 Position, inout vec3 Normal, inout vec2 TexCoord)\n"
 // 				 "{\n"
 // 				 "float t = 0.0;\n"
 // 				 "vec3 dist;\n"
 // 				 "vec3 right, up, forward, newright;\n"
 // 				 "\n"
 // 				 "#if defined(WAVE_SIN)\n" );
	const int funcType = deformv->func.type;
	for( size_t i = 0; i < numDeforms; i++, deformv++ ) {
		switch( deformv->type ) {
			case DEFORMV_WAVE: {
				if( funcType <= SHADER_FUNC_NONE || funcType > numSupportedFuncs || !funcs[funcType] ) {
					return str;
				}
			 // str = sdscatfmt(str, "#define DEFORMV_METHOD_%s 1", funcs[funcType]);
			 // 
			 // str = sdscatfmt(str, "#define DEFORMV_FUNC %s", funcs[funcType]);
			 // str = sdscatfmt(str, "#define DEFORMV_ARG_0 %f", deformv->func.args[0]);
			 // str = sdscatfmt(str, "#define DEFORMV_ARG_1 %f", deformv->func.args[1]);
			 // str = sdscatfmt(str, "#define DEFORMV_ARG_2 %f", deformv->func.args[2]);
			 // str = sdscatfmt(str, "#define DEFORMV_ARG_3 %f", deformv->func.args[3]);
				
				str = sdscatfmt(str, "#define DEFORMV_WAVE 1");
				str = sdscatfmt(str, "#define DEFORMV_FUNC %s", funcs[funcType]);
				str = sdscatfmt(str, "#define DEFORMV_CONSTANT vec4(%f,%f,%f,%f)", 
										deformv->func.args[0],
										deformv->func.args[1],
										deformv->func.args[2],
										deformv->func.args[3]
				);
				
				//str = sdscatfmt( str, "Position.xyz += %s(u_QF_ShaderTime,%f,%f,%f+%f*(Position.x+Position.y+Position.z),%f) * Normal.xyz;\n", funcs[funcType], 
				//         deformv->func.args[0],
				//				 deformv->func.args[1], 
				//				 deformv->func.args[2], 
				//				 deformv->func.args[3] ? deformv->args[0] : 0.0, 
				//				 deformv->func.args[3] );
				break;
			}
			case DEFORMV_MOVE: {
				if( funcType <= SHADER_FUNC_NONE || funcType > numSupportedFuncs || !funcs[funcType] ) {
					return str;
				}
		 // 	str = sdscatfmt(str, "#define DEFORMV_MOVE 1");
		 // 	str = sdscatfmt(str, "#define DEFORMV_METHOD_%s 1", funcs[funcType]);
				
				str = sdscatfmt(str, "#define DEFORMV_MOVE 1");
				str = sdscatfmt(str, "#define DEFORMV_FUNC %s", funcs[funcType]);
				str = sdscatfmt(str, "#define DEFORMV_CONSTANT vec4(%f,%f,%f,%f)", 
										deformv->func.args[0],
										deformv->func.args[1],
										deformv->func.args[2],
										deformv->func.args[3]
				);
				

				//str = sdscatfmt(str, "#define DEFORMV_ARG_0 %f", deformv->func.args[0]);
				//str = sdscatfmt(str, "#define DEFORMV_ARG_1 %f", deformv->func.args[1]);
				//str = sdscatfmt(str, "#define DEFORMV_ARG_2 %f", deformv->func.args[2]);
				//str = sdscatfmt(str, "#define DEFORMV_ARG_3 %f", deformv->func.args[3]);
				//str = sdscatfmt( str, "Position.xyz += %s(u_QF_ShaderTime,%f,%f,%f,%f) * vec3(%f, %f, %f);\n", funcs[funcType], 
				//         deformv->func.args[0], 
				//         deformv->func.args[1], 
				//         deformv->func.args[2],
				//				 deformv->func.args[3], 
				//				 deformv->args[0], 
				//				 deformv->args[1], 
				//				 deformv->args[2] );

				break;
			}
			case DEFORMV_BULGE:
				str = sdscatfmt(str, "#define DEFORMV_BULGE 1");
				str = sdscatfmt(str, "#define DEFORMV_CONSTANT vec4(%f,%f,%f,%f)", 
										deformv->func.args[0],
										deformv->func.args[1],
										deformv->func.args[2],
										deformv->func.args[3]
				);
			 // str = sdscatfmt( str,
			 // 				 "t = sin(TexCoord.s * %f + u_QF_ShaderTime * %f);\n"
			 // 				 "Position.xyz += max (-1.0 + %f, t) * %f * Normal.xyz;\n",
			 // 				 deformv->args[0], deformv->args[2], deformv->args[3], deformv->args[1] );

				break;
			case DEFORMV_AUTOSPRITE:
				str = sdscatfmt(str, "#define DEFORMV_AUTOSPRITE 1");
				//str = sdscatfmt( str,
				//				 "right = (1.0 + step(0.5, TexCoord.s) * -2.0) * u_QF_ViewAxis[1] * u_QF_MirrorSide;\n;"
				//				 "up = (1.0 + step(0.5, TexCoord.t) * -2.0) * u_QF_ViewAxis[2];\n"
				//				 "forward = -1.0 * u_QF_ViewAxis[0];\n"
				//				 "Position.xyz = a_SpritePoint.xyz + (right + up) * a_SpritePoint.w;\n"
				//				 "Normal.xyz = forward;\n"
				//				 "TexCoord.st = vec2(step(0.5, TexCoord.s),step(0.5, TexCoord.t));\n" );

				break;
			case DEFORMV_AUTOPARTICLE:
				str = sdscatfmt(str, "#define DEFORMV_AUTOPARTICLE 1");
				//str = sdscatfmt( str,
				//				 "right = (1.0 + TexCoord.s * -2.0) * u_QF_ViewAxis[1] * u_QF_MirrorSide;\n;"
				//				 "up = (1.0 + TexCoord.t * -2.0) * u_QF_ViewAxis[2];\n"
				//				 "forward = -1.0 * u_QF_ViewAxis[0];\n"
				//				 // prevent the particle from disappearing at large distances
				//				 "t = dot(a_SpritePoint.xyz + u_QF_EntityOrigin - u_QF_ViewOrigin, u_QF_ViewAxis[0]);\n"
				//				 "t = 1.5 + step(20.0, t) * t * 0.006;\n"
				//				 "Position.xyz = a_SpritePoint.xyz + (right + up) * t * a_SpritePoint.w;\n"
				//				 "Normal.xyz = forward;\n" );
				break;
			case DEFORMV_AUTOSPRITE2:
				str = sdscatfmt(str, "#define DEFORMV_AUTOSPRITE2 1");
				//str = sdscatfmt( str,
				//				 // local sprite axes
				//				 "right = QF_LatLong2Norm(a_SpriteRightUpAxis.xy) * u_QF_MirrorSide;\n"
				//				 "up = QF_LatLong2Norm(a_SpriteRightUpAxis.zw);\n"

				//				 // mid of quad to camera vector
				//				 "dist = u_QF_ViewOrigin - u_QF_EntityOrigin - a_SpritePoint.xyz;\n"

				//				 // filter any longest-axis-parts off the camera-direction
				//				 "forward = normalize(dist - up * dot(dist, up));\n"

				//				 // the right axis vector as it should be to face the camera
				//				 "newright = cross(up, forward);\n"

				//				 // rotate the quad vertex around the up axis vector
				//				 "t = dot(right, Position.xyz - a_SpritePoint.xyz);\n"
				//				 "Position.xyz += t * (newright - right);\n"
				//				 "Normal.xyz = forward;\n" );
				break;
			default:
				break;
		}
	}

	//str = sdscatfmt( str,
	//				 "#endif\n"
	//				 "}\n"
	//				 "\n" );

	return str;
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
void RP_PrecachePrograms( void )
{
//
//	int version;
//	char *buffer = NULL, *data, **ptr;
//	const char *token;
//	int handleBin;
//	size_t binaryCacheSize = 0;
//	bool isDefaultCache = false;
//	char tempbuf[MAX_TOKEN_CHARS];
//
//	R_LoadCacheFile( GLSL_CACHE_FILE_NAME, ( void ** )&buffer );
//	if( !buffer ) {
//		isDefaultCache = true;
//		r_glslbincache_storemode = FS_WRITE;
//
//		// load default glsl cache list, supposedly shipped with the game
//		R_LoadFile( GLSL_DEFAULT_CACHE_FILE_NAME, ( void ** )&buffer );
//		if( !buffer ) {
//			return;
//		}
//	}
//
//#define CLOSE_AND_DROP_BINARY_CACHE() do { \
//		ri.FS_FCloseFile( handleBin ); \
//		handleBin = 0; \
//		r_glslbincache_storemode = FS_WRITE; \
//	} while(0)
//
//	handleBin = 0;
//	if( glConfig.ext.get_program_binary && !isDefaultCache ) {
//		r_glslbincache_storemode = FS_APPEND;
//		if( ri.FS_FOpenFile( GLSL_BINARY_CACHE_FILE_NAME, &handleBin, FS_READ|FS_CACHE ) != -1 ) {
//			unsigned hash;
//
//			version = 0;
//			hash = 0;
//
//			ri.FS_Seek( handleBin, 0, FS_SEEK_END );
//			binaryCacheSize = ri.FS_Tell( handleBin );
//			ri.FS_Seek( handleBin, 0, FS_SEEK_SET );
//
//			ri.FS_Read( &version, sizeof( version ), handleBin );
//			ri.FS_Read( &hash, sizeof( hash ), handleBin );
//			
//			if( binaryCacheSize < 8 || version != GLSL_BITS_VERSION || hash != glConfig.versionHash ) {
//				CLOSE_AND_DROP_BINARY_CACHE();
//			}
//		}
//	}
//
//	data = buffer;
//	ptr = &data;
//
//	token = COM_Parse_r( tempbuf, sizeof( tempbuf ), ptr );
//	if( strcmp( token, rf.applicationName ) ) {
//		ri.Com_DPrintf( "Ignoring %s: unknown application name \"%s\", expected \"%s\"\n", 
//			token, rf.applicationName );
//		return;
//	}
//
//	token = COM_Parse_r( tempbuf, sizeof( tempbuf ), ptr );
//	version = atoi( token );
//	if( version != GLSL_BITS_VERSION ) {
//		// ignore cache files with mismatching version number
//		ri.Com_DPrintf( "Ignoring %s: found version %i, expected %i\n", version, GLSL_BITS_VERSION );
//	}
//	else {
//		while( 1 ) {
//			int type;
//			r_glslfeat_t lb, hb;
//			r_glslfeat_t features;
//			char name[256];
//			void *binary = NULL;
//			int binaryFormat = 0;
//			unsigned binaryLength = 0;
//			int binaryPos = 0;
//
//			// read program type
//			token = COM_Parse_r( tempbuf, sizeof( tempbuf ), ptr );
//			if( !token[0] ) {
//				break;
//			}
//			type = atoi( token );
//
//			// read lower bits
//			token = COM_ParseExt_r( tempbuf, sizeof( tempbuf ), ptr, false );
//			if( !token[0] ) {
//				break;
//			}
//			lb = atoi( token );
//
//			// read higher bits
//			token = COM_ParseExt_r( tempbuf, sizeof( tempbuf ), ptr, false );
//			if( !token[0] ) {
//				break;
//			}
//			hb = atoi( token );
//
//			// read program full name
//			token = COM_ParseExt_r( tempbuf, sizeof( tempbuf ), ptr, false );
//			if( !token[0] ) {
//				break;
//			}
//
//			Q_strncpyz( name, token, sizeof( name ) );
//			features = (hb << 32) | lb;
//#ifdef GL_ES_VERSION_2_0
//			if( isDefaultCache ) {
//				if( glConfig.ext.fragment_precision_high ) {
//					features |= GLSL_SHADER_COMMON_FRAGMENT_HIGHP;
//				}
//				else {
//					features &= ~GLSL_SHADER_COMMON_FRAGMENT_HIGHP;
//				}
//			}
//#endif
//
//			// read optional binary cache
//			token = COM_ParseExt_r( tempbuf, sizeof( tempbuf ), ptr, false );
//			if( handleBin && token[0] ) {
//				binaryPos = atoi( token );
//				if( binaryPos ) {
//					bool err = false;
//					
//					err = !err && ri.FS_Seek( handleBin, binaryPos, FS_SEEK_SET ) < 0;
//					err = !err && ri.FS_Read( &binaryFormat, sizeof( binaryFormat ), handleBin ) != sizeof( binaryFormat );
//					err = !err && ri.FS_Read( &binaryLength, sizeof( binaryLength ), handleBin ) != sizeof( binaryLength );
//					if( err || binaryLength >= binaryCacheSize ) {
//						binaryLength = 0;
//						CLOSE_AND_DROP_BINARY_CACHE();
//					}
//
//					if( binaryLength ) {
//						binary = R_Malloc( binaryLength );
//						if( binary != NULL && ri.FS_Read( binary, binaryLength, handleBin ) != (int)binaryLength ) {
//							R_Free( binary );
//							binary = NULL;
//							CLOSE_AND_DROP_BINARY_CACHE();
//						}
//					}
//				}
//			}
//
//			if( binary ) {
//				int elem;
//
//				ri.Com_DPrintf( "Loading binary program %s...\n", name );
//
//				elem = RP_RegisterProgramBinary( type, name, NULL, NULL, 0, features, 
//					binaryFormat, binaryLength, binary );
//
//				if( RP_GetProgramObject( elem ) == 0 ) {
//					// check whether the program actually exists
//					elem = 0;
//				}
//
//				if( !elem ) {
//					// rewrite this binary cache on exit
//					CLOSE_AND_DROP_BINARY_CACHE();
//				}
//				else {
//					struct glsl_program_s *program = r_glslprograms + elem - 1;
//					program->binaryCachePos = binaryPos;
//				}
//
//				R_Free( binary );
//				binary = NULL;
//
//				if( elem ) {
//					continue;
//				}
//				// fallthrough to regular registration
//			}
//			
//			ri.Com_DPrintf( "Loading program %s...\n", name );
//
//			RP_RegisterProgram( type, name, NULL, NULL, 0, features );
//		}
//	}
//
//#undef CLOSE_AND_DROP_BINARY_CACHE
//
//	R_FreeFile( buffer );
//
//	if( handleBin ) {
//		ri.FS_FCloseFile( handleBin );
//	}
}

/*
 * RP_StorePrecacheList
 *
 * Stores the list of known GLSL program permutations to file on the disk.
 * File format matches that expected by RP_PrecachePrograms.
 */
void RP_StorePrecacheList( void )
{
 // unsigned int i;
 // int handle, handleBin;
 // struct glsl_program_s *program;
 // unsigned dummy;

 // handle = 0;
 // if( ri.FS_FOpenFile( GLSL_CACHE_FILE_NAME, &handle, FS_WRITE | FS_CACHE ) == -1 ) {
 // 	Com_Printf( S_COLOR_YELLOW "Could not open %s for writing.\n", GLSL_CACHE_FILE_NAME );
 // 	return;
 // }

 // handleBin = 0;
 // if( glConfig.ext.get_program_binary ) {
 // 	if( ri.FS_FOpenFile( GLSL_BINARY_CACHE_FILE_NAME, &handleBin, r_glslbincache_storemode | FS_CACHE ) == -1 ) {
 // 		Com_Printf( S_COLOR_YELLOW "Could not open %s for writing.\n", GLSL_BINARY_CACHE_FILE_NAME );
 // 	} else if( r_glslbincache_storemode == FS_WRITE ) {
 // 		dummy = 0;
 // 		ri.FS_Write( &dummy, sizeof( dummy ), handleBin );

 // 		dummy = glConfig.versionHash;
 // 		ri.FS_Write( &dummy, sizeof( dummy ), handleBin );
 // 	} else {
 // 		ri.FS_Seek( handleBin, 0, FS_SEEK_END );
 // 	}
 // }

 // ri.FS_Printf( handle, "%s\n", rf.applicationName );
 // ri.FS_Printf( handle, "%i\n", GLSL_BITS_VERSION );

 // for( i = 0, program = r_glslprograms; i < r_numglslprograms; i++, program++ ) {
 // 	void *binary = NULL;
 // 	int binaryFormat = 0;
 // 	unsigned binaryLength = 0;
 // 	int binaryPos = 0;

 // 	if( *program->deformsKey ) {
 // 		continue;
 // 	}
 // 	if( !program->features ) {
 // 		continue;
 // 	}

 // 	if( handleBin ) {
 // 		if( r_glslbincache_storemode == FS_APPEND && program->binaryCachePos ) {
 // 			// this program is already cached
 // 			binaryPos = program->binaryCachePos;
 // 		} else {
 // 			binary = RP_GetProgramBinary( i + 1, &binaryFormat, &binaryLength );
 // 			if( binary ) {
 // 				binaryPos = ri.FS_Tell( handleBin );
 // 			}
 // 		}
 // 	}

 // 	ri.FS_Printf( handle, "%i %i %i \"%s\" %u\n", program->type, (int)( program->features & ULONG_MAX ), (int)( ( program->features >> 32 ) & ULONG_MAX ), program->name, binaryPos );

 // 	if( binary ) {
 // 		ri.FS_Write( &binaryFormat, sizeof( binaryFormat ), handleBin );
 // 		ri.FS_Write( &binaryLength, sizeof( binaryLength ), handleBin );
 // 		ri.FS_Write( binary, binaryLength, handleBin );
 // 		R_Free( binary );
 // 	}
 // }

 // ri.FS_FCloseFile( handle );
 // ri.FS_FCloseFile( handleBin );

 // if( handleBin && ri.FS_FOpenFile( GLSL_BINARY_CACHE_FILE_NAME, &handleBin, FS_UPDATE | FS_CACHE ) != -1 ) {
 // 	dummy = GLSL_BITS_VERSION;
 // 	ri.FS_Write( &dummy, sizeof( dummy ), handleBin );
 // 	ri.FS_FCloseFile( handleBin );
 // }
}

/*
 * RF_DeleteProgram
 */
static void RF_DeleteProgram( struct glsl_program_s *program )
{
	struct glsl_program_s *hash_next;

 // if( program->vertexShader ) {
 // 	qglDetachShader( program->object, program->vertexShader );
 // 	qglDeleteShader( program->vertexShader );
 // 	program->vertexShader = 0;
 // }

 // if( program->fragmentShader ) {
 // 	qglDetachShader( program->object, program->fragmentShader );
 // 	qglDeleteShader( program->fragmentShader );
 // 	program->fragmentShader = 0;
 // }

//	if( program->object )
//		qglDeleteProgram( program->object );

	if( program->name )
		R_Free( program->name );
	if( program->deformsKey )
		R_Free( program->deformsKey );

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
	[GLSL_PROGRAM_TYPE_COLORCORRECTION] = glsl_features_empty 
};

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

#define QF_BUILTIN_GLSL_MACROS "" \
"#if !defined(myhalf)\n" \
"//#if !defined(__GLSL_CG_DATA_TYPES)\n" \
"#define myhalf float\n" \
"#define myhalf2 vec2\n" \
"#define myhalf3 vec3\n" \
"#define myhalf4 vec4\n" \
"//#else\n" \
"//#define myhalf half\n" \
"//#define myhalf2 half2\n" \
"//#define myhalf3 half3\n" \
"//#define myhalf4 half4\n" \
"//#endif\n" \
"#endif\n" \
"#ifdef GL_ES\n" \
"#define qf_lowp_float lowp float\n" \
"#define qf_lowp_vec2 lowp vec2\n" \
"#define qf_lowp_vec3 lowp vec3\n" \
"#define qf_lowp_vec4 lowp vec4\n" \
"#else\n" \
"#define qf_lowp_float float\n" \
"#define qf_lowp_vec2 vec2\n" \
"#define qf_lowp_vec3 vec3\n" \
"#define qf_lowp_vec4 vec4\n" \
"#endif\n"

#define QF_BUILTIN_GLSL_MACROS_GLSL120 "" \
"#define qf_varying varying\n" \
"#define qf_flat_varying varying\n" \
"#ifdef VERTEX_SHADER\n" \
"# define qf_FrontColor gl_FrontColor\n" \
"# define qf_attribute attribute\n" \
"#endif\n" \
"#ifdef FRAGMENT_SHADER\n" \
"# define qf_FrontColor gl_Color\n" \
"# define qf_FragColor gl_FragColor\n" \
"#endif\n" \
"#define qf_texture texture2D\n" \
"#define qf_textureLod texture2DLod\n" \
"#define qf_textureCube textureCube\n" \
"#define qf_textureArray texture2DArray\n" \
"#define qf_texture3D texture3D\n" \
"#define qf_textureOffset(a,b,c,d) texture2DOffset(a,b,ivec2(c,d))\n" \
"#define qf_shadow shadow2D\n" \
"\n"

#define QF_BUILTIN_GLSL_MACROS_GLSL130                                  \
	""                                                                  \
	"precision highp float;\n"                                          \
	"#ifdef VERTEX_SHADER\n"                                            \
	"  layout(location = 0) out myhalf4 qf_FrontColor;\n"                                    \
	"# define qf_varying out\n"                                         \
	"# define qf_flat_varying flat out\n"                               \
	"# define qf_attribute in\n"                                        \
	"#endif\n"                                                          \
	"#ifdef FRAGMENT_SHADER\n"                                          \
	"  layout(location = 0) in myhalf4 qf_FrontColor;\n"                                     \
	"  layout(location = 1) out myhalf4 qf_FragColor;\n"                                     \
	"# define qf_varying in\n"                                          \
	"# define qf_flat_varying flat in\n"                                \
	"#endif\n"                                                          \
	"#define qf_texture texture\n"                                      \
	"#define qf_textureCube texture\n"                                  \
	"#define qf_textureLod textureLod\n"                                \
	"#define qf_textureArray texture\n"                                 \
	"#define qf_texture3D texture\n"                                    \
	"#define qf_textureOffset(a,b,c,d) textureOffset(a,b,ivec2(c,d))\n" \
	"#define qf_shadow texture\n"                                       \
	"\n"

#define QF_BUILTIN_GLSL_MACROS_GLSL100ES "" \
"#define qf_varying varying\n" \
"#define qf_flat_varying varying\n" \
"#ifdef VERTEX_SHADER\n" \
"# define qf_attribute attribute\n" \
"#endif\n" \
"#ifdef FRAGMENT_SHADER\n" \
"# if defined(GL_FRAGMENT_PRECISION_HIGH) && defined(QF_FRAGMENT_PRECISION_HIGH)\n" \
"   precision highp float;\n" \
"# else\n" \
"   precision mediump float;\n" \
"# endif\n" \
"# ifdef GL_EXT_texture_array\n" \
"   precision lowp sampler2DArray;\n" \
"# endif\n" \
"# ifdef GL_OES_texture_3D\n" \
"   precision lowp sampler3D;\n" \
"# endif\n" \
"# ifdef GL_EXT_shadow_samplers\n" \
"   precision lowp sampler2DShadow;\n" \
"# endif\n" \
"# define qf_FragColor gl_FragColor\n" \
"#endif\n" \
" qf_varying myhalf4 qf_FrontColor;\n" \
"#define qf_texture texture2D\n" \
"#define qf_textureLod texture2DLod\n" \
"#define qf_textureCube textureCube\n" \
"#define qf_textureArray texture2DArray\n" \
"#define qf_texture3D texture3D\n" \
"#define qf_shadow shadow2DEXT\n" \
"\n"

#define QF_BUILTIN_GLSL_MACROS_GLSL300ES "" \
"#ifdef VERTEX_SHADER\n" \
"# define qf_varying out\n" \
"# define qf_flat_varying flat out\n" \
"# define qf_attribute in\n" \
"#endif\n" \
"#ifdef FRAGMENT_SHADER\n" \
"# ifdef QF_FRAGMENT_PRECISION_HIGH\n" \
"   precision highp float;\n" \
"# else\n" \
"   precision mediump float;\n" \
"# endif\n" \
"  precision lowp sampler2DArray;\n" \
"  precision lowp sampler3D;\n" \
"  precision lowp sampler2DShadow;\n" \
"  layout(location = 0) out vec4 qf_FragColor;\n" \
"# define qf_varying in\n" \
"# define qf_flat_varying flat in\n" \
"#endif\n" \
" qf_varying myhalf4 qf_FrontColor;\n" \
"#define qf_texture texture\n" \
"#define qf_textureLod textureLod\n" \
"#define qf_textureCube texture\n" \
"#define qf_textureArray texture\n" \
"#define qf_texture3D texture\n" \
"#define qf_shadow texture\n" \
"\n"

#define QF_GLSL_PI "" \
"#ifndef M_PI\n" \
"#define M_PI 3.14159265358979323846\n" \
"#endif\n" \
"#ifndef M_TWOPI\n" \
"#define M_TWOPI 6.28318530717958647692\n" \
"#endif\n"

#define QF_BUILTIN_GLSL_CONSTANTS \
QF_GLSL_PI \
"\n" \
"#ifndef MAX_UNIFORM_INSTANCES\n" \
"#define MAX_UNIFORM_INSTANCES " STR_TOSTR( MAX_GLSL_UNIFORM_INSTANCES ) "\n" \
"#endif\n"

#define QF_BUILTIN_GLSL_UNIFORMS \
"uniform vec3 u_QF_ViewOrigin;\n" \
"uniform mat3 u_QF_ViewAxis;\n" \
"uniform float u_QF_MirrorSide;\n" \
"uniform vec3 u_QF_EntityOrigin;\n" \
"uniform float u_QF_ShaderTime;\n"

#define QF_BUILTIN_GLSL_QUAT_TRANSFORM_OVERLOAD \
"#ifdef QF_DUAL_QUAT_TRANSFORM_TANGENT\n" \
"void QF_VertexDualQuatsTransform_Tangent(inout vec4 Position, inout vec3 Normal, inout vec3 Tangent)\n" \
"#else\n" \
"void QF_VertexDualQuatsTransform(inout vec4 Position, inout vec3 Normal)\n" \
"#endif\n" \
"{\n" \
"	ivec4 Indices = ivec4(a_BonesIndices * 2.0);\n" \
"	vec4 DQReal = u_DualQuats[Indices.x];\n" \
"	vec4 DQDual = u_DualQuats[Indices.x + 1];\n" \
"#if QF_NUM_BONE_INFLUENCES >= 2\n" \
"	DQReal *= a_BonesWeights.x;\n" \
"	DQDual *= a_BonesWeights.x;\n" \
"	vec4 DQReal1 = u_DualQuats[Indices.y];\n" \
"	vec4 DQDual1 = u_DualQuats[Indices.y + 1];\n" \
"	float Scale = mix(-1.0, 1.0, step(0.0, dot(DQReal1, DQReal))) * a_BonesWeights.y;\n" \
"	DQReal += DQReal1 * Scale;\n" \
"	DQDual += DQDual1 * Scale;\n" \
"#if QF_NUM_BONE_INFLUENCES >= 3\n" \
"	DQReal1 = u_DualQuats[Indices.z];\n" \
"	DQDual1 = u_DualQuats[Indices.z + 1];\n" \
"	Scale = mix(-1.0, 1.0, step(0.0, dot(DQReal1, DQReal))) * a_BonesWeights.z;\n" \
"	DQReal += DQReal1 * Scale;\n" \
"	DQDual += DQDual1 * Scale;\n" \
"#if QF_NUM_BONE_INFLUENCES >= 4\n" \
"	DQReal1 = u_DualQuats[Indices.w];\n" \
"	DQDual1 = u_DualQuats[Indices.w + 1];\n" \
"	Scale = mix(-1.0, 1.0, step(0.0, dot(DQReal1, DQReal))) * a_BonesWeights.w;\n" \
"	DQReal += DQReal1 * Scale;\n" \
"	DQDual += DQDual1 * Scale;\n" \
"#endif // QF_NUM_BONE_INFLUENCES >= 4\n" \
"#endif // QF_NUM_BONE_INFLUENCES >= 3\n" \
"	float Len = 1.0 / length(DQReal);\n" \
"	DQReal *= Len;\n" \
"	DQDual *= Len;\n" \
"#endif // QF_NUM_BONE_INFLUENCES >= 2\n" \
"	Position.xyz += (cross(DQReal.xyz, cross(DQReal.xyz, Position.xyz) + Position.xyz * DQReal.w + DQDual.xyz) +\n" \
"		DQDual.xyz*DQReal.w - DQReal.xyz*DQDual.w) * 2.0;\n" \
"	Normal += cross(DQReal.xyz, cross(DQReal.xyz, Normal) + Normal * DQReal.w) * 2.0;\n" \
"#ifdef QF_DUAL_QUAT_TRANSFORM_TANGENT\n" \
"	Tangent += cross(DQReal.xyz, cross(DQReal.xyz, Tangent) + Tangent * DQReal.w) * 2.0;\n" \
"#endif\n" \
"}\n" \
"\n"

#define QF_BUILTIN_GLSL_QUAT_TRANSFORM \
"qf_attribute vec4 a_BonesIndices, a_BonesWeights;\n" \
"uniform vec4 u_DualQuats[MAX_UNIFORM_BONES*2];\n" \
"\n" \
QF_BUILTIN_GLSL_QUAT_TRANSFORM_OVERLOAD \
"#define QF_DUAL_QUAT_TRANSFORM_TANGENT\n" \
QF_BUILTIN_GLSL_QUAT_TRANSFORM_OVERLOAD \
"#undef QF_DUAL_QUAT_TRANSFORM_TANGENT\n"

#define QF_BUILTIN_GLSL_INSTANCED_TRANSFORMS \
"#if defined(APPLY_INSTANCED_ATTRIB_TRANSFORMS)\n" \
"qf_attribute vec4 a_InstanceQuat, a_InstancePosAndScale;\n" \
"#elif defined(GL_ARB_draw_instanced) || (defined(GL_ES) && (__VERSION__ >= 300))\n" \
"uniform vec4 u_InstancePoints[MAX_UNIFORM_INSTANCES*2];\n" \
"#define a_InstanceQuat u_InstancePoints[gl_InstanceID*2]\n" \
"#define a_InstancePosAndScale u_InstancePoints[gl_InstanceID*2+1]\n" \
"#else\n" \
"uniform vec4 u_InstancePoints[2];\n" \
"#define a_InstanceQuat u_InstancePoints[0]\n" \
"#define a_InstancePosAndScale u_InstancePoints[1]\n" \
"#endif // APPLY_INSTANCED_ATTRIB_TRANSFORMS\n" \
"\n" \
"void QF_InstancedTransform(inout vec4 Position, inout vec3 Normal)\n" \
"{\n" \
"	Position.xyz = (cross(a_InstanceQuat.xyz,\n" \
"		cross(a_InstanceQuat.xyz, Position.xyz) + Position.xyz*a_InstanceQuat.w)*2.0 +\n" \
"		Position.xyz) * a_InstancePosAndScale.w + a_InstancePosAndScale.xyz;\n" \
"	Normal = cross(a_InstanceQuat.xyz, cross(a_InstanceQuat.xyz, Normal) + Normal*a_InstanceQuat.w)*2.0 + Normal;\n" \
"}\n" \
"\n"

// We have to use these #ifdefs here because #defining prototypes
// of these functions to nothing results in a crash on Intel GPUs.
#define QF_BUILTIN_GLSL_TRANSFORM_VERTS \
"void QF_TransformVerts(inout vec4 Position, inout vec3 Normal, inout vec2 TexCoord)\n" \
"{\n" \
"#	ifdef QF_NUM_BONE_INFLUENCES\n" \
"		QF_VertexDualQuatsTransform(Position, Normal);\n" \
"#	endif\n" \
"#	ifdef QF_APPLY_DEFORMVERTS\n" \
"		QF_DeformVerts(Position, Normal, TexCoord);\n" \
"#	endif\n" \
"#	ifdef APPLY_INSTANCED_TRANSFORMS\n" \
"		QF_InstancedTransform(Position, Normal);\n" \
"#	endif\n" \
"}\n" \
"\n" \
"void QF_TransformVerts_Tangent(inout vec4 Position, inout vec3 Normal, inout vec3 Tangent, inout vec2 TexCoord)\n" \
"{\n" \
"#	ifdef QF_NUM_BONE_INFLUENCES\n" \
"		QF_VertexDualQuatsTransform_Tangent(Position, Normal, Tangent);\n" \
"#	endif\n" \
"#	ifdef QF_APPLY_DEFORMVERTS\n" \
"		QF_DeformVerts(Position, Normal, TexCoord);\n" \
"#	endif\n" \
"#	ifdef APPLY_INSTANCED_TRANSFORMS\n" \
"		QF_InstancedTransform(Position, Normal);\n" \
"#	endif\n" \
"}\n" \
"\n"

#define QF_GLSL_WAVEFUNCS \
"\n" \
QF_GLSL_PI \
"\n" \
"#ifndef WAVE_SIN\n" \
"float QF_WaveFunc_Sin(float x)\n" \
"{\n" \
"return sin(fract(x) * M_TWOPI);\n" \
"}\n" \
"float QF_WaveFunc_Triangle(float x)\n" \
"{\n" \
"x = fract(x);\n" \
"return step(x, 0.25) * x * 4.0 + (2.0 - 4.0 * step(0.25, x) * step(x, 0.75) * x) + ((step(0.75, x) * x - 0.75) * 4.0 - 1.0);\n" \
"}\n" \
"float QF_WaveFunc_Square(float x)\n" \
"{\n" \
"return step(fract(x), 0.5) * 2.0 - 1.0;\n" \
"}\n" \
"float QF_WaveFunc_Sawtooth(float x)\n" \
"{\n" \
"return fract(x);\n" \
"}\n" \
"float QF_WaveFunc_InverseSawtooth(float x)\n" \
"{\n" \
"return 1.0 - fract(x);\n" \
"}\n" \
"\n" \
"#define WAVE_SIN(time,base,amplitude,phase,freq) (((base)+(amplitude)*QF_WaveFunc_Sin((phase)+(time)*(freq))))\n" \
"#define WAVE_TRIANGLE(time,base,amplitude,phase,freq) (((base)+(amplitude)*QF_WaveFunc_Triangle((phase)+(time)*(freq))))\n" \
"#define WAVE_SQUARE(time,base,amplitude,phase,freq) (((base)+(amplitude)*QF_WaveFunc_Square((phase)+(time)*(freq))))\n" \
"#define WAVE_SAWTOOTH(time,base,amplitude,phase,freq) (((base)+(amplitude)*QF_WaveFunc_Sawtooth((phase)+(time)*(freq))))\n" \
"#define WAVE_INVERSESAWTOOTH(time,base,amplitude,phase,freq) (((base)+(amplitude)*QF_WaveFunc_InverseSawtooth((phase)+(time)*(freq))))\n" \
"#endif\n" \
"\n"

#define QF_GLSL_MATH \
"#define QF_LatLong2Norm(ll) vec3(cos((ll).y) * sin((ll).x), sin((ll).y) * sin((ll).x), cos((ll).x))\n" \
"\n"


#define PARSER_MAX_STACKDEPTH 16

static bool __RF_AppendShaderFromFile( sds *str, const char *rootFile, const char *fileName, int stackDepth, int programType, r_glslfeat_t features )
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
	sds includeFilePath = sdsempty();
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
			( *str ) = sdscat( *str, startBuf );
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

		sdsclear( includeFilePath );
		includeFilePath = sdsMakeRoomFor( includeFilePath, strlen( fileName ) + 1 + strlen( token ) + 1 );

		// tempFilename = R_Malloc( tempFilenameSize );

		if( *token != '/' ) {
			includeFilePath = sdscat( includeFilePath, fileName );
			COM_StripFilename( includeFilePath );
		} else {
			token++;
			includeFilePath = sdscat( includeFilePath, rootFile );
			COM_StripFilename( includeFilePath );
		}
		sdsupdatelen( includeFilePath );
		includeFilePath = sdscat( includeFilePath, ( *includeFilePath ? "/" : "" ) );
		includeFilePath = sdscat( includeFilePath, token );
		if( __RF_AppendShaderFromFile( str, rootFile, includeFilePath, stackDepth + 1, programType, features ) ) {
			error = true;
			goto done;
		}
	}
	if( startBuf ) {
		*str = sdscat( *str, startBuf );
	}
done:
	R_Free( fileContents );
	sdsfree( includeFilePath );
	return error;
}

static bool RF_AppendShaderFromFile( sds *str, const char *fileName, int programType, r_glslfeat_t features )
{
	return __RF_AppendShaderFromFile( str, fileName, fileName, 1, programType, features );
}

int RP_GetProgramObject( int elem )
{
	assert(0);
	return 0;
	//if( elem < 1 ) {
	//	return 0;
	//}
	//return r_glslprograms[elem - 1].object;
}

/*
 * RP_GetProgramBinary
 *
 * Retrieves the binary from the program object
 */
static void *RP_GetProgramBinary( int elem, int *format, unsigned *length )
{
	//void *binary;
	//struct glsl_program_s *program = r_glslprograms + elem - 1;
	//GLenum GLFormat;
	//GLint GLlength;
	//GLint linked = 0;

	//if( !glConfig.ext.get_program_binary ) {
	//	return NULL;
	//}
	//if( !program->object ) {
	//	return NULL;
	//}

	//qglGetProgramiv( program->object, GL_OBJECT_LINK_STATUS_ARB, &linked );
	//if( !linked ) {
	//	return NULL;
	//}

	//// FIXME: need real pointer to glGetProgramiv here,
	//// aliasing to glGetObjectParameterivARB doesn't work
	//qglGetProgramiv( program->object, GL_PROGRAM_BINARY_LENGTH, &GLlength );
	//if( !GLlength ) {
	//	return NULL;
	//}

	//binary = R_Malloc( GLlength );
	//qglGetProgramBinary( program->object, GLlength, NULL, &GLFormat, binary );

	//*format = GLFormat;
	//*length = GLlength;
	assert(0);
	return NULL;
}

void RP_ProgramList_f( void )
{
	Com_Printf( "------------------\n" );
	size_t i;
	struct glsl_program_s *program;
	for( i = 0, program = r_glslprograms; i < MAX_GLSL_PROGRAMS; i++, program++ ) {
		if( !program->name )
			break;

		sds fullName = sdsempty();
		fullName = sdscat( fullName, program->name );
		for( feature_iter_t iter = { .it = glsl_programtypes_features[program->type], .bits = program->features }; __R_IsValidFeatureIter( &iter ); iter = __R_NextFeature( iter ) ) {
			if( iter.it->suffix ) {
				fullName = sdscat( fullName, iter.it->suffix );
			}
		}
		Com_Printf( " %3i %s", i + 1, fullName );
		sdsfree( fullName );

		if( *program->deformsKey ) {
			Com_Printf( " dv:%s", program->deformsKey );
		}
		Com_Printf( "\n" );
	}
	Com_Printf( "%i programs total\n", i );
}

/*
 * RP_UpdateShaderUniforms
 */
void RP_UpdateShaderUniforms( int elem,
							  float shaderTime,
							  const vec3_t entOrigin,
							  const vec3_t entDist,
							  const uint8_t *entityColor,
							  const uint8_t *constColor,
							  const float *rgbGenFuncArgs,
							  const float *alphaGenFuncArgs,
							  const mat4_t texMatrix )
{
	GLfloat m[9];
	struct glsl_program_s *program = r_glslprograms + elem - 1;

	if( entOrigin ) {
		if( program->loc.EntityOrigin >= 0 )
			qglUniform3fvARB( program->loc.EntityOrigin, 1, entOrigin );
		if( program->loc.builtin.EntityOrigin >= 0 )
			qglUniform3fvARB( program->loc.builtin.EntityOrigin, 1, entOrigin );
	}

	if( program->loc.EntityDist >= 0 && entDist )
		qglUniform3fvARB( program->loc.EntityDist, 1, entDist );
	if( program->loc.EntityColor >= 0 && entityColor )
		qglUniform4fARB( program->loc.EntityColor, entityColor[0] * 1.0 / 255.0, entityColor[1] * 1.0 / 255.0, entityColor[2] * 1.0 / 255.0, entityColor[3] * 1.0 / 255.0 );

	if( program->loc.ShaderTime >= 0 )
		qglUniform1fARB( program->loc.ShaderTime, shaderTime );
	if( program->loc.builtin.ShaderTime >= 0 )
		qglUniform1fARB( program->loc.builtin.ShaderTime, shaderTime );

	if( program->loc.ConstColor >= 0 && constColor )
		qglUniform4fARB( program->loc.ConstColor, constColor[0] * 1.0 / 255.0, constColor[1] * 1.0 / 255.0, constColor[2] * 1.0 / 255.0, constColor[3] * 1.0 / 255.0 );
	if( program->loc.RGBGenFuncArgs >= 0 && rgbGenFuncArgs )
		qglUniform4fvARB( program->loc.RGBGenFuncArgs, 1, rgbGenFuncArgs );
	if( program->loc.AlphaGenFuncArgs >= 0 && alphaGenFuncArgs )
		qglUniform4fvARB( program->loc.AlphaGenFuncArgs, 1, alphaGenFuncArgs );

	// FIXME: this looks shit...
	if( program->loc.TextureMatrix >= 0 ) {
		m[0] = texMatrix[0], m[1] = texMatrix[4];
		m[2] = texMatrix[1], m[3] = texMatrix[5];
		m[4] = texMatrix[12], m[5] = texMatrix[13];

		qglUniform4fvARB( program->loc.TextureMatrix, 2, m );
	}
}

/*
 * RP_UpdateViewUniforms
 */
void RP_UpdateViewUniforms( int elem,
							const mat4_t modelviewMatrix,
							const mat4_t modelviewProjectionMatrix,
							const vec3_t viewOrigin,
							const mat3_t viewAxis,
							const float mirrorSide,
							int viewport[4],
							float zNear,
							float zFar )
{
	struct glsl_program_s *program = r_glslprograms + elem - 1;

	if( program->loc.ModelViewMatrix >= 0 ) {
		qglUniformMatrix4fvARB( program->loc.ModelViewMatrix, 1, GL_FALSE, modelviewMatrix );
	}
	if( program->loc.ModelViewProjectionMatrix >= 0 ) {
		qglUniformMatrix4fvARB( program->loc.ModelViewProjectionMatrix, 1, GL_FALSE, modelviewProjectionMatrix );
	}

	if( program->loc.ZRange >= 0 ) {
		qglUniform2fARB( program->loc.ZRange, zNear, zFar );
	}

	if( viewOrigin ) {
		if( program->loc.ViewOrigin >= 0 )
			qglUniform3fvARB( program->loc.ViewOrigin, 1, viewOrigin );
		if( program->loc.builtin.ViewOrigin >= 0 )
			qglUniform3fvARB( program->loc.builtin.ViewOrigin, 1, viewOrigin );
	}

	if( viewAxis ) {
		if( program->loc.ViewAxis >= 0 )
			qglUniformMatrix3fvARB( program->loc.ViewAxis, 1, GL_FALSE, viewAxis );
		if( program->loc.builtin.ViewAxis >= 0 )
			qglUniformMatrix3fvARB( program->loc.builtin.ViewAxis, 1, GL_FALSE, viewAxis );
	}

	if( program->loc.Viewport >= 0 ) {
		qglUniform4ivARB( program->loc.Viewport, 1, viewport );
	}

	if( program->loc.MirrorSide >= 0 )
		qglUniform1fARB( program->loc.MirrorSide, mirrorSide );
	if( program->loc.builtin.MirrorSide >= 0 )
		qglUniform1fARB( program->loc.builtin.MirrorSide, mirrorSide );
}

/*
 * RP_UpdateBlendMixUniform
 *
 * The first component corresponds to RGB, the second to ALPHA.
 * Whenever the program needs to scale source colors, the mask needs
 * to be used in the following manner:
 * color *= mix(myhalf4(1.0), myhalf4(scale), u_BlendMix.xxxy);
 */
void RP_UpdateBlendMixUniform( int elem, vec2_t blendMix )
{
	struct glsl_program_s *program = r_glslprograms + elem - 1;

	if( program->loc.BlendMix >= 0 ) {
		qglUniform2fvARB( program->loc.BlendMix, 1, blendMix );
	}
}

/*
 * RP_UpdateSoftParticlesUniforms
 */
void RP_UpdateSoftParticlesUniforms( int elem, float scale )
{
	struct glsl_program_s *program = r_glslprograms + elem - 1;

	if( program->loc.SoftParticlesScale >= 0 ) {
		qglUniform1fARB( program->loc.SoftParticlesScale, scale );
	}
}

/*
 * RP_UpdateDiffuseLightUniforms
 */
void RP_UpdateDiffuseLightUniforms( int elem, const vec3_t lightDir, const vec4_t lightAmbient, const vec4_t lightDiffuse )
{
	struct glsl_program_s *program = r_glslprograms + elem - 1;

	if( program->loc.LightDir >= 0 && lightDir )
		qglUniform3fvARB( program->loc.LightDir, 1, lightDir );
	if( program->loc.LightAmbient >= 0 && lightAmbient )
		qglUniform3fARB( program->loc.LightAmbient, lightAmbient[0], lightAmbient[1], lightAmbient[2] );
	if( program->loc.LightDiffuse >= 0 && lightDiffuse )
		qglUniform3fARB( program->loc.LightDiffuse, lightDiffuse[0], lightDiffuse[1], lightDiffuse[2] );
}

/*
 * RP_UpdateMaterialUniforms
 */
void RP_UpdateMaterialUniforms( int elem, float offsetmappingScale, float glossIntensity, float glossExponent )
{
	struct glsl_program_s *program = r_glslprograms + elem - 1;

	if( program->loc.GlossFactors >= 0 )
		qglUniform2fARB( program->loc.GlossFactors, glossIntensity, glossExponent );
	if( program->loc.OffsetMappingScale >= 0 )
		qglUniform1fARB( program->loc.OffsetMappingScale, offsetmappingScale );
}

/*
 * RP_UpdateDistortionUniforms
 */
void RP_UpdateDistortionUniforms( int elem, bool frontPlane )
{
	struct glsl_program_s *program = r_glslprograms + elem - 1;

	if( program->loc.FrontPlane >= 0 )
		qglUniform1fARB( program->loc.FrontPlane, frontPlane ? 1 : -1 );
}

/*
 * RP_UpdateTextureUniforms
 */
void RP_UpdateTextureUniforms( int elem, int TexWidth, int TexHeight )
{
	struct glsl_program_s *program = r_glslprograms + elem - 1;

	if( program->loc.TextureParams >= 0 )
		qglUniform4fARB( program->loc.TextureParams, TexWidth, TexHeight, TexWidth ? 1.0 / TexWidth : 1.0, TexHeight ? 1.0 / TexHeight : 1.0 );
}

/*
 * RP_UpdateOutlineUniforms
 */
void RP_UpdateOutlineUniforms( int elem, float projDistance )
{
	struct glsl_program_s *program = r_glslprograms + elem - 1;

	if( program->loc.OutlineHeight >= 0 )
		qglUniform1fARB( program->loc.OutlineHeight, projDistance );
	if( program->loc.OutlineCutOff >= 0 )
		qglUniform1fARB( program->loc.OutlineCutOff, max( 0, r_outlines_cutoff->value ) );
}

/*
 * RP_UpdateFogUniforms
 */
void RP_UpdateFogUniforms( int elem, byte_vec4_t color, float clearDist, float opaqueDist, cplane_t *fogPlane, cplane_t *eyePlane, float eyeDist )
{
	GLfloat fog_color[3] = { 0, 0, 0 };
	struct glsl_program_s *program = r_glslprograms + elem - 1;

	VectorScale( color, ( 1.0 / 255.0 ), fog_color );

	if( program->loc.Fog.Color >= 0 )
		qglUniform3fvARB( program->loc.Fog.Color, 1, fog_color );
	if( program->loc.Fog.ScaleAndEyeDist >= 0 )
		qglUniform2fARB( program->loc.Fog.ScaleAndEyeDist, 1.0 / ( opaqueDist - clearDist ), eyeDist );
	if( program->loc.Fog.Plane >= 0 )
		qglUniform4fARB( program->loc.Fog.Plane, fogPlane->normal[0], fogPlane->normal[1], fogPlane->normal[2], fogPlane->dist );
	if( program->loc.Fog.EyePlane >= 0 )
		qglUniform4fARB( program->loc.Fog.EyePlane, eyePlane->normal[0], eyePlane->normal[1], eyePlane->normal[2], eyePlane->dist );
}

/*
 * RP_UpdateDynamicLightsUniforms
 */
unsigned int RP_UpdateDynamicLightsUniforms( int elem, const superLightStyle_t *superLightStyle, const vec3_t entOrigin, const mat3_t entAxis, unsigned int dlightbits )
{
	int i, n, c;
	dlight_t *dl;
	float colorScale = mapConfig.mapLightColorScale;
	vec3_t dlorigin, tvec, dlcolor;
	struct glsl_program_s *program = r_glslprograms + elem - 1;
	bool identityAxis = Matrix3_Compare( entAxis, axis_identity );
	vec4_t shaderColor[4];

	if( superLightStyle ) {
		int i;
		GLfloat rgb[3];
		static float deluxemapOffset[( MAX_LIGHTMAPS + 3 ) & ( ~3 )];

		for( i = 0; i < MAX_LIGHTMAPS && superLightStyle->lightmapStyles[i] != 255; i++ ) {
			VectorCopy( rsc.lightStyles[superLightStyle->lightmapStyles[i]].rgb, rgb );
			if( mapConfig.lightingIntensity )
				VectorScale( rgb, mapConfig.lightingIntensity, rgb );

			if( program->loc.LightstyleColor[i] >= 0 )
				qglUniform3fvARB( program->loc.LightstyleColor[i], 1, rgb );
			if( program->loc.DeluxemapOffset >= 0 )
				deluxemapOffset[i] = superLightStyle->stOffset[i][0];
		}

		if( i && ( program->loc.DeluxemapOffset >= 0 ) )
			qglUniform4fvARB( program->loc.DeluxemapOffset, ( i + 3 ) / 4, deluxemapOffset );
	}

	if( dlightbits ) {
		memset( shaderColor, 0, sizeof( vec4_t ) * 3 );
		Vector4Set( shaderColor[3], 1.0f, 1.0f, 1.0f, 1.0f );
		n = 0;
		for( i = 0; i < MAX_DLIGHTS; i++ ) {
			dl = rsc.dlights + i;
			if( !dl->intensity ) {
				continue;
			}
			if( program->loc.DynamicLightsPosition[n] < 0 ) {
				break;
			}

			VectorSubtract( dl->origin, entOrigin, dlorigin );
			if( !identityAxis ) {
				VectorCopy( dlorigin, tvec );
				Matrix3_TransformVector( entAxis, tvec, dlorigin );
			}
			VectorScale( dl->color, colorScale, dlcolor );

			qglUniform3fvARB( program->loc.DynamicLightsPosition[n], 1, dlorigin );

			c = n & 3;
			shaderColor[0][c] = dlcolor[0];
			shaderColor[1][c] = dlcolor[1];
			shaderColor[2][c] = dlcolor[2];
			shaderColor[3][c] = 1.0f / dl->intensity;

			// DynamicLightsDiffuseAndInvRadius is transposed for SIMD, but it's still 4x4
			if( c == 3 ) {
				qglUniform4fvARB( program->loc.DynamicLightsDiffuseAndInvRadius[n >> 2], 4, shaderColor[0] );
				memset( shaderColor, 0, sizeof( vec4_t ) * 3 );
				Vector4Set( shaderColor[3], 1.0f, 1.0f, 1.0f, 1.0f );
			}

			n++;
			dlightbits &= ~( 1 << i );
			if( !dlightbits ) {
				break;
			}
		}

		if( n & 3 ) {
			qglUniform4fvARB( program->loc.DynamicLightsDiffuseAndInvRadius[n >> 2], 4, shaderColor[0] );
			memset( shaderColor, 0, sizeof( vec4_t ) * 3 ); // to set to zero for the remaining lights
			Vector4Set( shaderColor[3], 1.0f, 1.0f, 1.0f, 1.0f );
			n = ALIGN( n, 4 );
		}

		if( program->loc.NumDynamicLights >= 0 ) {
			qglUniform1iARB( program->loc.NumDynamicLights, n );
		}

		for( ; n < MAX_DLIGHTS; n += 4 ) {
			if( program->loc.DynamicLightsPosition[n] < 0 ) {
				break;
			}
			qglUniform4fvARB( program->loc.DynamicLightsDiffuseAndInvRadius[n >> 2], 4, shaderColor[0] );
		}
	}

	return 0;
}

/*
 * RP_UpdateTexGenUniforms
 */
void RP_UpdateTexGenUniforms( int elem, const mat4_t reflectionMatrix, const mat4_t vectorMatrix )
{
	struct glsl_program_s *program = r_glslprograms + elem - 1;

	if( program->loc.ReflectionTexMatrix >= 0 ) {
		mat3_t m;
		memcpy( &m[0], &reflectionMatrix[0], 3 * sizeof( vec_t ) );
		memcpy( &m[3], &reflectionMatrix[4], 3 * sizeof( vec_t ) );
		memcpy( &m[6], &reflectionMatrix[8], 3 * sizeof( vec_t ) );
		qglUniformMatrix3fvARB( program->loc.ReflectionTexMatrix, 1, GL_FALSE, m );
	}
	if( program->loc.VectorTexMatrix >= 0 )
		qglUniformMatrix4fvARB( program->loc.VectorTexMatrix, 1, GL_FALSE, vectorMatrix );
}

/*
 * RP_UpdateShadowsUniforms
 */
void RP_UpdateShadowsUniforms( int elem, int numShadows, const shadowGroup_t **groups, const mat4_t objectMatrix, const vec3_t objectOrigin, const mat3_t objectAxis )
{
	int i;
	const shadowGroup_t *group;
	mat4_t matrix;
	vec4_t alpha;
	struct glsl_program_s *program = r_glslprograms + elem - 1;

	assert( groups != NULL );
	assert( numShadows <= GLSL_SHADOWMAP_LIMIT );

	if( numShadows > GLSL_SHADOWMAP_LIMIT ) {
		numShadows = GLSL_SHADOWMAP_LIMIT;
	}

	for( i = 0; i < numShadows; i++ ) {
		group = groups[i];

		if( program->loc.ShadowmapTextureParams[i] >= 0 ) {
			qglUniform4fARB( program->loc.ShadowmapTextureParams[i], group->viewportSize[0], group->viewportSize[1], 1.0f / group->textureSize[0], 1.0 / group->textureSize[1] );
		}

		if( program->loc.ShadowmapMatrix[i] >= 0 ) {
			Matrix4_Multiply( group->cameraProjectionMatrix, objectMatrix, matrix );
			qglUniformMatrix4fvARB( program->loc.ShadowmapMatrix[i], 1, GL_FALSE, matrix );
		}

		if( program->loc.ShadowAlpha[i >> 2] >= 0 ) {
			alpha[i & 3] = group->alpha;
			if( ( i & 3 ) == 3 ) {
				qglUniform4fvARB( program->loc.ShadowAlpha[i >> 2], 1, alpha );
			}
		}

		if( program->loc.ShadowDir[i] >= 0 ) {
			vec4_t lightDir;
			Matrix3_TransformVector( objectAxis, group->lightDir, lightDir );
			lightDir[3] = group->projDist;
			qglUniform4fvARB( program->loc.ShadowDir[i], 1, lightDir );
		}

		if( program->loc.ShadowEntityDist[i] >= 0 ) {
			vec3_t tmp, entDist;
			VectorSubtract( group->origin, objectOrigin, tmp );
			Matrix3_TransformVector( objectAxis, tmp, entDist );
			qglUniform3fvARB( program->loc.ShadowEntityDist[i], 1, entDist );
		}
	}

	if( ( i & 3 ) && ( program->loc.ShadowAlpha[i >> 2] >= 0 ) ) {
		qglUniform4fvARB( program->loc.ShadowAlpha[i >> 2], 1, alpha );
	}
}

/*
 * RP_UpdateBonesUniforms
 *
 * Set uniform values for animation dual quaternions
 */
void RP_UpdateBonesUniforms( int elem, unsigned int numBones, dualquat_t *animDualQuat )
{
	struct glsl_program_s *program = r_glslprograms + elem - 1;

	if( numBones > glConfig.maxGLSLBones ) {
		return;
	}
	if( program->loc.DualQuats < 0 ) {
		return;
	}
	qglUniform4fvARB( program->loc.DualQuats, numBones * 2, &animDualQuat[0][0] );
}

/*
 * RP_UpdateInstancesUniforms
 *
 * Set uniform values for instance points (quaternion + xyz + scale)
 */
void RP_UpdateInstancesUniforms( int elem, unsigned int numInstances, instancePoint_t *instances )
{
	struct glsl_program_s *program = r_glslprograms + elem - 1;

	if( numInstances > MAX_GLSL_UNIFORM_INSTANCES ) {
		numInstances = MAX_GLSL_UNIFORM_INSTANCES;
	}
	if( program->loc.InstancePoints < 0 ) {
		return;
	}
	qglUniform4fvARB( program->loc.InstancePoints, numInstances * 2, &instances[0][0] );
}

void RP_UpdateDrawFlatUniforms( int elem, const vec3_t wallColor, const vec3_t floorColor )
{
	struct glsl_program_s *program = r_glslprograms + elem - 1;

	if( program->loc.WallColor >= 0 )
		qglUniform3fARB( program->loc.WallColor, wallColor[0], wallColor[1], wallColor[2] );
	if( program->loc.FloorColor >= 0 )
		qglUniform3fARB( program->loc.FloorColor, floorColor[0], floorColor[1], floorColor[2] );
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
	

static inline struct pipeline_hash_s* __resolvePipeline(struct glsl_program_s *program, hash_t hash) {
	const size_t startIndex = hash % PIPELINE_LAYOUT_HASH_SIZE;
	size_t index = startIndex;
	do {
		if(program->pipelines[index].hash == hash || program->pipelines[index].hash == 0) {
			program->pipelines[index].hash = hash;
			return &program->pipelines[index];
		}
		index = (index + 1) % PIPELINE_LAYOUT_HASH_SIZE;
	} while(index != startIndex);
	return NULL;
}

struct pipeline_hash_s *RP_ResolvePipeline( struct glsl_program_s *program, struct pipeline_layout_config_s *def ) {
	
	hash_t hash = HASH_INITIAL_VALUE;
	hash = hash_u32(hash, def->attrib );
	hash = hash_u32(hash, def->halfAttrib);
	hash = hash_data(hash, &def->inputAssembly, sizeof(NriInputAssemblyDesc)); 
	hash = hash_data(hash, &def->rasterization, sizeof(NriRasterizationDesc)); 

	struct pipeline_hash_s* pipeline = __resolvePipeline(program, hash);
	if(pipeline->pipeline) {
		return pipeline; // pipeline is present in slot
	}

	NriShaderDesc shaderDesc[16] = {0};
	NriGraphicsPipelineDesc graphicsPipelineDesc = {
		.shaders = shaderDesc,
		.rasterization = def->rasterization,
		.inputAssembly = def->inputAssembly
	};

	assert( rsh.nri.api == NriGraphicsAPI_VULKAN );

	NriVertexStreamDesc vertexStreamDesc[32] = {0};
  NriVertexAttributeDesc vertexAttributeDesc[32] = {};
	struct attrib_init_s {
		vattribbit_t attr;
		size_t numChannels;
		const char *d3dAttribute;
		uint32_t vkAttribute;
	} attributes[] = { 
		{ VATTRIB_POSITION_BIT, 4, "POSITION", 0 }, 
		{ VATTRIB_NORMAL_BIT, 4, "NORMAL", 0 }, 
		{ VATTRIB_SVECTOR_BIT, 4, "TANGENT", 0 }, 
		{ VATTRIB_TEXCOORDS_BIT, 2, "TEXCOORD_0", 0 }, 
		//{ VATTRIB_LMCOORDS0_BIT, 2, "position", 0 }, 
		//{ VATTRIB_LMCOORDS1_BIT, 2, "position", 0 }, 
		//{ VATTRIB_LMCOORDS2_BIT, 2, "position", 0 }, 
		//{ VATTRIB_LMCOORDS3_BIT, 2, "position", 0 }, 
	};
	uint32_t streamIdx = 0;
	uint32_t attribIndex = 0;
	uint32_t bindingSlot = 0;
	for( size_t i = 0; i < Q_ARRAY_COUNT( attributes ); i++ ) {
		if( def->attrib & attributes[i].attr ) {
			vertexAttributeDesc[attribIndex].offset = 0;
			vertexAttributeDesc[attribIndex].streamIndex = streamIdx;
			vertexAttributeDesc[attribIndex].d3d = ( NriVertexAttributeD3D ){ attributes[i].d3dAttribute, 0 };
			vertexAttributeDesc[attribIndex++].vk = ( NriVertexAttributeVK ){ attributes[i].vkAttribute };

			vertexStreamDesc[streamIdx].bindingSlot = bindingSlot++;
			vertexStreamDesc[streamIdx++].stride = ( ( def->halfAttrib & attributes[i].attr ) ? sizeof( uint16_t ) : sizeof( uint32_t ) ) * attributes[i].numChannels;
		}
	}

	NriVertexInputDesc vertexInputDesc = {
		.attributes = vertexAttributeDesc,
		.attributeNum = attribIndex,
		.streams = vertexStreamDesc,
		.streamNum = streamIdx
	};

	graphicsPipelineDesc.vertexInput = &vertexInputDesc;
	if( program->shaderBin[GLSL_STAGE_VERTEX].bin && 
		program->shaderBin[GLSL_STAGE_FRAGMENT].bin ) {
		shaderDesc[0] = (NriShaderDesc) {
			.size = program->shaderBin[GLSL_STAGE_VERTEX].size,
			.stage = NriStageBits_FRAGMENT_SHADER,
			.bytecode = program->shaderBin[GLSL_STAGE_VERTEX].bin,
			.entryPointName = "main"
		};

		shaderDesc[1] = (NriShaderDesc) {
			.size = program->shaderBin[GLSL_STAGE_FRAGMENT].size,
			.stage = NriStageBits_FRAGMENT_SHADER,
			.bytecode = program->shaderBin[GLSL_STAGE_FRAGMENT].bin,
			.entryPointName = "main"
		};
		graphicsPipelineDesc.shaderNum = 2;
	} else {
		return NULL;
	}
	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateGraphicsPipeline( rsh.nri.device, &graphicsPipelineDesc, &pipeline->pipeline ) );
	return pipeline;
}

static NriDescriptorRangeDesc *__FindAndInsertNriDescriptorRange( const SpvReflectDescriptorBinding *binding, NriDescriptorRangeDesc **ranges )
{
	for( size_t i = 0; i < arrlen( ranges ); i++ ) {
		if( ( *ranges )[i].baseRegisterIndex == binding->binding ) {
			return &( *ranges )[i];
		}
	}
	const size_t insertIndex = arraddnindex( ranges, 1 );
	memset( &( *ranges )[insertIndex], 0, sizeof( NriDescriptorRangeDesc ) );
	return &( *ranges )[insertIndex];
}

static bool __InsertReflectionDescriptorSet( struct glsl_program_s *program, const struct descriptor_reflection_s* reflection)
{
	const size_t startIndex = reflection->hash % PIPELINE_LAYOUT_HASH_SIZE;
	size_t index = startIndex;
	do {
		if( program->descriptorReflection[index].hash == reflection->hash || program->pipelines[index].hash == 0 ) {
			memcpy(&program->descriptorReflection[index], reflection, sizeof(struct descriptor_reflection_s));
			return true;
		}
		index = ( index + 1 ) % PIPELINE_LAYOUT_HASH_SIZE;
	} while( index != startIndex );
	// we've run out of slots
	return false;
}

static const struct descriptor_reflection_s* __ReflectDescriptorSet(const struct glsl_program_s *program,const struct glsl_descriptor_handle_s* handle) {
	const size_t startIndex = handle->hash % DESCRIPTOR_REFLECTION_HASH_SIZE;
	size_t index = startIndex;
	do {
		if( program->descriptorReflection[index].hash == handle->hash ) {
			return &program->descriptorReflection[index];
		} else if( program->descriptorReflection[index].hash == 0 ) {
			return NULL;
		}
		index = (index + 1) % DESCRIPTOR_REFLECTION_HASH_SIZE;
	} while(index != startIndex);
	return NULL;

}

void RP_BindDescriptorSets(struct frame_cmd_buffer_s* cmd, struct glsl_program_s *program, struct glsl_descriptor_data_s *data, size_t numDescriptorData )
{
	struct glsl_descriptor_commit_s {
		NriDescriptor const *descriptors[DESCRIPTOR_MAX_BINDINGS];
		uint32_t hashes[DESCRIPTOR_MAX_BINDINGS];
		uint32_t descriptorChangeMask;
	} commit[DESCRIPTOR_SET_MAX] = {0};
	uint32_t setsChangeMask = 0;
	
	for(size_t i = 0; i < numDescriptorData; i++) {
		const struct descriptor_reflection_s* refl = __ReflectDescriptorSet(program, &data[i].handle);
		const uint32_t setIndex = refl->setIndex;
		const uint32_t slotIndex = refl->baseRegisterIndex + data[i].registerOffset;

		setsChangeMask |= ( 1u << setIndex );
		commit[setIndex].descriptorChangeMask |= ( 1u << slotIndex );
		switch(refl->slotType) {
			case GLSL_REFLECTION_IMAGE:
				commit[setIndex].hashes[slotIndex] |= data->image->cookie;
				commit[setIndex].descriptors[slotIndex] = data->image->descriptor;
				break;
		}
	}
	
	for(uint32_t setIndex = 0, setMask = setsChangeMask; setMask > 0 && setIndex++; setMask >>= 1u) {	
		struct glsl_descriptor_commit_s* c = &commit[setIndex];
		hash_t descHash = HASH_INITIAL_VALUE;
		for( uint32_t descMask = c->descriptorChangeMask, descIndex = 0; descMask; descMask >>= 1u, descIndex++ ) {
			descHash = hash_u64(descHash, c->hashes[descIndex]);
		}
		struct descriptor_set_result_s result = ResolveDescriptorSet( &rsh.nri, cmd, program->descriptorSetAlloc[setIndex], descHash );
		if(!result.found) {
			for( uint32_t descMask = c->descriptorChangeMask, descIndex = 0; descMask; descMask >>= 1u, descIndex++ ) {
				NriDescriptorRangeUpdateDesc updateDesc = { 0 };
				updateDesc.descriptors = &c->descriptors[descIndex];
				updateDesc.descriptorNum = 1;
				rsh.nri.coreI.UpdateDescriptorRanges(result.set, descIndex, 1, &updateDesc);
			}

		}
		rsh.nri.coreI.CmdSetDescriptorSet( cmd->cmd, setIndex, result.set, NULL );
	}

}

struct glsl_program_s *RP_ResolveProgram( int type, const char *name, const char *deformsKey, const deformv_t *deforms, int numDeforms, r_glslfeat_t features )
{
	if( type <= GLSL_PROGRAM_TYPE_NONE || type >= GLSL_PROGRAM_TYPE_MAXTYPE )
		return 0;

	assert( !deforms || deformsKey );

	if( !deforms )
		deformsKey = "";

	const uint32_t hashIndex = hash_u64( HASH_INITIAL_VALUE, features ) % GLSL_PROGRAMS_HASH_SIZE;
	for( struct glsl_program_s *program = r_glslprograms_hash[type][hashIndex]; program; program = program->hash_next ) {
		if( ( program->features == features ) && !strcmp( program->deformsKey, deformsKey ) ) {
			return program;
		}
	}

	if( r_numglslprograms == MAX_GLSL_PROGRAMS ) {
		Com_Printf( S_COLOR_YELLOW "RP_RegisterProgram: GLSL programs limit exceeded\n" );
		return 0;
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
	program = r_glslprograms + r_numglslprograms++;
	sds featuresStr = sdsempty();
	sds fullName = sdsnew( name );

	ri.Com_DPrintf( "Registering GLSL program %s\n", fullName );

	bool error = false;
	struct {
		glsl_program_stage_t stage;
		sds source;
	} stages[] = {
		{ .stage = GLSL_STAGE_VERTEX, .source = sdsempty() },
		{ .stage = GLSL_STAGE_FRAGMENT, .source = sdsempty() },
	};
	{
		sds filePath = sdsempty();
		for( size_t i = 0; i < Q_ARRAY_COUNT( stages ); i++ ) {
			stages[i].source = sdscatfmt( stages[i].source, "#version 440 \n" );
			switch( stages[i].stage ) {
				case GLSL_STAGE_VERTEX:
					stages[i].source = sdscatfmt( stages[i].source, "#define VERTEX_SHADER\n" );
					break;
				case GLSL_STAGE_FRAGMENT:
					stages[i].source = sdscatfmt( stages[i].source, "#define FRAGMENT_SHADER\n" );
					break;
				default:
					assert( 0 );
					break;
			}
			stages[i].source = sdscatfmt( stages[i].source, QF_BUILTIN_GLSL_MACROS );
			stages[i].source = sdscatfmt( stages[i].source, "#define MAX_UNIFORM_BONES %i\n", r_maxglslbones->integer );
			stages[i].source = sdscat( stages[i].source, QF_BUILTIN_GLSL_MACROS_GLSL130 );
			stages[i].source = sdscat( stages[i].source, QF_BUILTIN_GLSL_CONSTANTS );
			stages[i].source = sdscatfmt( stages[i].source, QF_GLSL_MATH );
			switch( stages[i].stage ) {
				case GLSLANG_STAGE_VERTEX: {
					stages[i].source = R_AppendGLSLDeformv( stages[i].source, deforms, numDeforms );
					stages[i].source = sdscat( stages[i].source, QF_GLSL_WAVEFUNCS );
					if( features & GLSL_SHADER_COMMON_BONE_TRANSFORMS ) {
						stages[i].source = sdscat( stages[i].source, QF_BUILTIN_GLSL_QUAT_TRANSFORM );
					}
					if( features & ( GLSL_SHADER_COMMON_INSTANCED_TRANSFORMS | GLSL_SHADER_COMMON_INSTANCED_ATTRIB_TRANSFORMS ) ) {
						stages[i].source = sdscatfmt( stages[i].source, "%s\n", QF_BUILTIN_GLSL_INSTANCED_TRANSFORMS );
					}
					stages[i].source = sdscatfmt( stages[i].source, "%s\n", QF_BUILTIN_GLSL_TRANSFORM_VERTS );
					break;
				}
				case GLSL_STAGE_FRAGMENT:
					break;
				default:
					assert( 0 );
					break;
			}
			for( feature_iter_t iter = { .it = glsl_programtypes_features[type], .bits = features }; __R_IsValidFeatureIter( &iter ); iter = __R_NextFeature( iter ) ) {
				featuresStr = sdscatfmt( featuresStr, "%s\n", iter.it->define );
				if( fullName ) {
					fullName = sdscatfmt( fullName, "%s\n", iter.it->suffix );
				}
			}

			sdsclear( filePath );
			filePath = sdscatfmt( filePath, "glsl_nri/%s.%s.glsl", name, RP_GLSLStageToShaderPrefix( stages[i].stage ) );
			error = RF_AppendShaderFromFile( &stages[i].source, filePath, type, features );
			if( error ) {
				break;
			}
		}
		sdsfree( filePath );
	}

	SpvReflectDescriptorSet** reflectionDescSets = NULL;
	SpvReflectBlockVariable* reflectionBlockSets[1] = {0};

	NriDescriptorSetDesc descriptorSetDesc[DESCRIPTOR_SET_MAX] = {0};
	NriDescriptorRangeDesc* descRangeDescs[DESCRIPTOR_SET_MAX] = {0};
	NriPushConstantDesc descPushConstantDescs = {0};

	for( size_t stageIdx = 0; stageIdx < Q_ARRAY_COUNT( stages ); stageIdx++ ) {
		const glslang_input_t input = { 
										.language = GLSLANG_SOURCE_GLSL,
										.stage = __RP_GLStageToSlang( stages[stageIdx].stage ),
										.client = GLSLANG_CLIENT_VULKAN,
										.client_version = GLSLANG_TARGET_VULKAN_1_2,
										.target_language = GLSLANG_TARGET_SPV,
										.target_language_version = GLSLANG_TARGET_SPV_1_5,
										.code = stages[stageIdx].source,
										.default_version = 450,
										.default_profile = GLSLANG_CORE_PROFILE,
										.force_default_version_and_profile = false,
										.forward_compatible = false,
										.messages = GLSLANG_MSG_DEFAULT_BIT,
										.resource = glslang_default_resource() };
		glslang_shader_t *shader = glslang_shader_create( &input );
		glslang_program_t *glslang_program = NULL;
		if( !glslang_shader_preprocess( shader, &input ) ) {
			sds errFilePath = sdscatfmt( sdsempty(), "logs/shader_err/%s.%s.glsl", name, RP_GLSLStageToShaderPrefix( stages[stageIdx].stage ) );
			const char *infoLog = glslang_shader_get_info_log( shader );
			const char *debugLogs = glslang_shader_get_info_debug_log( shader );
			Com_Printf( S_COLOR_YELLOW "GLSL preprocess failed %s\n", fullName );
			Com_Printf( S_COLOR_YELLOW "%s\n", infoLog );
			Com_Printf( S_COLOR_YELLOW "%s\n", debugLogs );
			Com_Printf( S_COLOR_YELLOW "dump shader: %s\n", errFilePath );

			sds shaderErr = sdsempty();
			shaderErr = sdscatfmt( shaderErr, "%s\n", input.code );
			shaderErr = sdscatfmt( shaderErr, "----------- Preprocessing Failed -----------\n" );
			shaderErr = sdscatfmt( shaderErr, "%s\n", infoLog );
			shaderErr = sdscatfmt( shaderErr, "%s\n", debugLogs );
			__RP_writeTextToFile( errFilePath, shaderErr );
			sdsfree( shaderErr );
			sdsfree( errFilePath );

			error = true;
			goto shader_done;
		}

		if( !glslang_shader_parse( shader, &input ) ) {
			sds errFilePath = sdscatfmt( sdsempty(), "logs/shader_err/%s.%s.glsl", name, RP_GLSLStageToShaderPrefix( stages[stageIdx].stage ) );
			const char *infoLog = glslang_shader_get_info_log( shader );
			const char *debugLogs = glslang_shader_get_info_debug_log( shader );
			// const char *preprocessedCode = glslang_shader_get_preprocessed_code( shader );

			Com_Printf( S_COLOR_YELLOW "GLSL parsing failed %s\n", fullName );
			Com_Printf( S_COLOR_YELLOW "%s\n", infoLog );
			Com_Printf( S_COLOR_YELLOW "%s\n", debugLogs );
			Com_Printf( S_COLOR_YELLOW "dump shader: %s\n", errFilePath );

			sds shaderErr = sdsempty();
			shaderErr = sdscatfmt( shaderErr, "%s\n", input.code );
			shaderErr = sdscatfmt( shaderErr, "----------- Parsing Failed -----------\n" );
			shaderErr = sdscatfmt( shaderErr, "%s\n", infoLog );
			shaderErr = sdscatfmt( shaderErr, "%s\n", debugLogs );
			__RP_writeTextToFile( errFilePath, shaderErr );
			sdsfree( shaderErr );
			sdsfree( errFilePath );

			error = true;
			goto shader_done;
		}
		glslang_program = glslang_program_create();
		glslang_program_add_shader( glslang_program, shader );

		if( !glslang_program_link( glslang_program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT ) ) {
			sds errFilePath = sdscatfmt( sdsempty(), "logs/shader_err/%s.%s.glsl", name, RP_GLSLStageToShaderPrefix( stages[stageIdx].stage ) );

			const char *infoLogs = glslang_program_get_info_log( glslang_program );
			const char *debugLogs = glslang_program_get_info_debug_log( glslang_program );
			// const char *preprocessedCode = glslang_shader_get_preprocessed_code( shader );

			Com_Printf( S_COLOR_YELLOW "GLSL linking failed %s\n", fullName );
			Com_Printf( S_COLOR_YELLOW "%s\n", infoLogs );
			Com_Printf( S_COLOR_YELLOW "%s\n", debugLogs );
			Com_Printf( S_COLOR_YELLOW "dump shader: %s\n", errFilePath );

			sds shaderErr = sdsempty();
			shaderErr = sdscatfmt( shaderErr, "%s\n", input.code );
			shaderErr = sdscatfmt( shaderErr, "----------- Linking Failed -----------\n" );
			shaderErr = sdscatfmt( shaderErr, "%s\n", infoLogs );
			shaderErr = sdscatfmt( shaderErr, "%s\n", debugLogs );
			__RP_writeTextToFile( errFilePath, shaderErr );
			sdsfree( shaderErr );
			sdsfree( errFilePath );

			error = true;
			goto shader_done;
		}

		const char *spirv_messages = glslang_program_SPIRV_get_messages( glslang_program );
		if( spirv_messages ) {
			Com_Printf( S_COLOR_BLUE "(%s) %s\b", name, spirv_messages );
		}

		glslang_program_SPIRV_generate( glslang_program, __RP_GLStageToSlang( stages[stageIdx].stage ) );
		size_t binSize = glslang_program_SPIRV_get_size( glslang_program ) * sizeof( uint32_t );
		struct shader_bin_data_s *binData = &program->shaderBin[program->shaderStageSize++];
		binData->bin = R_Malloc( binSize );
		glslang_program_SPIRV_get( glslang_program, (uint32_t *)binData->bin );
		binData->stage = stages[stageIdx].stage;

		SpvReflectShaderModule module = { 0 };
		SpvReflectResult result = spvReflectCreateShaderModule( binSize / sizeof( uint32_t ), binData->bin, &module );
		assert( result == SPV_REFLECT_RESULT_SUCCESS );

		// configure push constant sets for pipeline layout
		{
			uint32_t pushConstantCount = 0;
			result = spvReflectEnumeratePushConstantBlocks( &module, &pushConstantCount, NULL );
			assert( result == SPV_REFLECT_RESULT_SUCCESS );
			if(pushConstantCount > 0) {
				assert( pushConstantCount == 1 ); // lets only support 1 push constant

				result = spvReflectEnumeratePushConstantBlocks( &module, &pushConstantCount, reflectionBlockSets );
				assert( result == SPV_REFLECT_RESULT_SUCCESS );
				descPushConstantDescs.size = reflectionBlockSets[0]->size;
				switch( stages[stageIdx].stage ) {
					case GLSLANG_STAGE_VERTEX:
						descPushConstantDescs.shaderStages |= NriStageBits_VERTEX_SHADER;
						break;
					case GLSL_STAGE_FRAGMENT:
						descPushConstantDescs.shaderStages |= NriStageBits_FRAGMENT_SHADER;
						break;
					default:
						assert( false );
						break;
				}
			}
		}

		// configure descriptor sets for pipeline layout
		{
			uint32_t descCount = 0;
			result = spvReflectEnumerateDescriptorSets( &module, &descCount, NULL );
			assert( result == SPV_REFLECT_RESULT_SUCCESS );
			arrsetlen( reflectionDescSets, descCount );

			result = spvReflectEnumerateDescriptorSets( &module, &descCount, reflectionDescSets );
			assert( result == SPV_REFLECT_RESULT_SUCCESS );

			for( size_t i_set = 0; i_set < descCount; i_set++ ) {
				const SpvReflectDescriptorSet *reflection = reflectionDescSets[i_set];
				
				// create the descriptor allocator
				if(program->descriptorSetAlloc[reflection->set] == NULL) {
					program->descriptorSetAlloc[reflection->set] = malloc(sizeof(struct descriptor_set_allloc_s));
					memset(program->descriptorSetAlloc[reflection->set], 0, sizeof(struct descriptor_set_allloc_s));
				}
				struct descriptor_set_allloc_s* const descAlloc = program->descriptorSetAlloc[reflection->set];
				descAlloc->config.layout = program->layout;
				descAlloc->config.setIndex = reflection->set;

				for( size_t i_binding = 0; i_binding < reflection->binding_count; i_binding++ ) {
					const SpvReflectDescriptorBinding *reflectionBinding = reflection->bindings[i_binding];
					assert(reflection->set < DESCRIPTOR_SET_MAX);
					struct descriptor_reflection_s reflc = {0};
					reflc.hash = Create_DescriptorHandle(reflectionBinding->name).hash;
					reflc.setIndex = reflection->set;
					reflc.baseRegisterIndex = reflectionBinding->binding;

					//struct glsl_descriptor_handle_s handle = Create_DescriptorHandle(reflectionBinding->name);
					//, reflection->set, reflectionBinding->binding );
					
					NriDescriptorRangeDesc *rangeDesc = __FindAndInsertNriDescriptorRange( reflectionBinding, &descRangeDescs[reflection->set] );
					rangeDesc->descriptorNum = reflectionBinding->array.dims_count;
					rangeDesc->baseRegisterIndex = reflectionBinding->binding;
					rangeDesc->isArray = reflectionBinding->array.dims_count > 0;
					reflc.isArray = reflectionBinding->array.dims_count > 0;
					reflc.dimCount = max(1, reflectionBinding->array.dims_count);
					const uint32_t bindingCount = max( reflectionBinding->array.dims_count, 1 );
					switch( reflectionBinding->descriptor_type ) {
						case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
							rangeDesc->descriptorType = NriDescriptorType_SAMPLER;
							descAlloc->config.samplerMaxNum += bindingCount;
							break;
						case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
							rangeDesc->descriptorType = NriDescriptorType_TEXTURE;
							descAlloc->config.textureMaxNum += bindingCount;
							break;
						case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
							rangeDesc->descriptorType = NriDescriptorType_BUFFER;
							descAlloc->config.bufferMaxNum += bindingCount;
							break;
						case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
							rangeDesc->descriptorType = NriDescriptorType_STORAGE_TEXTURE;
							descAlloc->config.storageTextureMaxNum += bindingCount;
							reflc.slotType = GLSL_REFLECTION_IMAGE;
							break;
						case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
							rangeDesc->descriptorType = NriDescriptorType_STORAGE_BUFFER;
							descAlloc->config.storageBufferMaxNum += bindingCount;
							break;
						case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
							rangeDesc->descriptorType = NriDescriptorType_CONSTANT_BUFFER;
							descAlloc->config.constantBufferMaxNum += bindingCount;
							break;
						case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
							rangeDesc->descriptorType = NriDescriptorType_STRUCTURED_BUFFER;
							descAlloc->config.structuredBufferMaxNum += bindingCount;
							break;
						case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
							rangeDesc->descriptorType = NriDescriptorType_STORAGE_STRUCTURED_BUFFER;
							descAlloc->config.storageStructuredBufferMaxNum += bindingCount;
							break;
						case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
						case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
						case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
						case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
							assert( false );
							break;
					}
					__InsertReflectionDescriptorSet(program, &reflc);
					// rangeDesc->descriptorType
					switch( stages[stageIdx].stage ) {
						case GLSLANG_STAGE_VERTEX:
							rangeDesc->shaderStages |= NriStageBits_VERTEX_SHADER;
							break;
						case GLSL_STAGE_FRAGMENT:
							rangeDesc->shaderStages |= NriStageBits_FRAGMENT_SHADER;
							break;
						default:
							assert( false );
							break;
					}
				}
			}
		}

	shader_done:
		glslang_shader_delete( shader );
		if( glslang_program ) {
			glslang_program_delete( glslang_program );
		}
	}
	arrfree(reflectionDescSets);
	NriPipelineLayoutDesc pipelineLayoutdesc = { 0 };
	for(size_t i = 0; i < DESCRIPTOR_SET_MAX; i++) {
		if(descRangeDescs[i]) {
			descriptorSetDesc[pipelineLayoutdesc.descriptorSetNum].registerSpace = i;
			descriptorSetDesc[pipelineLayoutdesc.descriptorSetNum].rangeNum = arrlen(descRangeDescs[i]);
			descriptorSetDesc[pipelineLayoutdesc.descriptorSetNum++].ranges = descRangeDescs[i];
		}
	}
	pipelineLayoutdesc.descriptorSets = descriptorSetDesc;
	if( descPushConstantDescs.size > 0 ) {
		pipelineLayoutdesc.pushConstantNum = 1;
		pipelineLayoutdesc.pushConstants = &descPushConstantDescs;
	}

	NRI_ABORT_ON_FAILURE(rsh.nri.coreI.CreatePipelineLayout(rsh.nri.device, &pipelineLayoutdesc, &program->layout));
	
	for(size_t  i = 0;  i < DESCRIPTOR_SET_MAX; i++) {
		arrfree( descRangeDescs[i] );
	}

	sdsfree( fullName );
	for( size_t i = 0; i < Q_ARRAY_COUNT( stages ); i++ ) {
		sdsfree( stages[i].source );
	}

	program->type = type;
	program->features = features;
	program->name = R_CopyString( name );
	program->deformsKey = R_CopyString( deformsKey ? deformsKey : "" );

	if( !program->hash_next ) {
		program->hash_next = r_glslprograms_hash[type][hashIndex];
		r_glslprograms_hash[type][hashIndex] = program;
	}

	return program;
}

int RP_RegisterProgram( int type, const char *name, const char *deformsKey, const deformv_t *deforms, int numDeforms, r_glslfeat_t features )
{
	return 0;
}

/*
* RP_GetUniformLocations
*/
static void RP_GetUniformLocations( struct glsl_program_s *program )
{
//	char tmp[1024];
//	unsigned int i;
//	int		locBaseTexture,
//			locNormalmapTexture,
//			locGlossTexture,
//			locDecalTexture,
//			locEntityDecalTexture,
//			locLightmapTexture[MAX_LIGHTMAPS],
//			locDuDvMapTexture,
//			locReflectionTexture,
//			locRefractionTexture,
//			locShadowmapTexture[GLSL_SHADOWMAP_LIMIT],
//			locCelShadeTexture,
//			locCelLightTexture,
//			locDiffuseTexture,
//			locStripesTexture,
//			locDepthTexture,
//			locYUVTextureY,
//			locYUVTextureU,
//			locYUVTextureV,
//			locColorLUT
//			;
//
//	memset( &program->loc, -1, sizeof( program->loc ) );
//
//	program->loc.ModelViewMatrix = qglGetUniformLocationARB( program->object, "u_ModelViewMatrix" );
//	program->loc.ModelViewProjectionMatrix = qglGetUniformLocationARB( program->object, "u_ModelViewProjectionMatrix" );
//
//	program->loc.ZRange = qglGetUniformLocationARB( program->object, "u_ZRange" );
//
//	program->loc.ViewOrigin = qglGetUniformLocationARB( program->object, "u_ViewOrigin" );
//	program->loc.ViewAxis = qglGetUniformLocationARB( program->object, "u_ViewAxis" );
//
//	program->loc.MirrorSide = qglGetUniformLocationARB( program->object, "u_MirrorSide" );
//
//	program->loc.Viewport = qglGetUniformLocationARB( program->object, "u_Viewport" );
//
//	program->loc.LightDir = qglGetUniformLocationARB( program->object, "u_LightDir" );
//	program->loc.LightAmbient = qglGetUniformLocationARB( program->object, "u_LightAmbient" );
//	program->loc.LightDiffuse = qglGetUniformLocationARB( program->object, "u_LightDiffuse" );
//
//	program->loc.TextureMatrix = qglGetUniformLocationARB( program->object, "u_TextureMatrix" );
//	
//	locBaseTexture = qglGetUniformLocationARB( program->object, "u_BaseTexture" );
//	locNormalmapTexture = qglGetUniformLocationARB( program->object, "u_NormalmapTexture" );
//	locGlossTexture = qglGetUniformLocationARB( program->object, "u_GlossTexture" );
//	locDecalTexture = qglGetUniformLocationARB( program->object, "u_DecalTexture" );
//	locEntityDecalTexture = qglGetUniformLocationARB( program->object, "u_EntityDecalTexture" );
//
//	locDuDvMapTexture = qglGetUniformLocationARB( program->object, "u_DuDvMapTexture" );
//	locReflectionTexture = qglGetUniformLocationARB( program->object, "u_ReflectionTexture" );
//	locRefractionTexture = qglGetUniformLocationARB( program->object, "u_RefractionTexture" );
//
//	for( i = 0; i < GLSL_SHADOWMAP_LIMIT; i++ ) {
//		locShadowmapTexture[i] = qglGetUniformLocationARB( program->object, 
//			va_r( tmp, sizeof( tmp ), "u_ShadowmapTexture%i", i ) );
//		if( locShadowmapTexture[i] < 0 )
//			break;
//	}
//
//	locCelShadeTexture = qglGetUniformLocationARB( program->object, "u_CelShadeTexture" );
//	locCelLightTexture = qglGetUniformLocationARB( program->object, "u_CelLightTexture" );
//	locDiffuseTexture = qglGetUniformLocationARB( program->object, "u_DiffuseTexture" );
//	locStripesTexture = qglGetUniformLocationARB( program->object, "u_StripesTexture" );
//
//	locDepthTexture = qglGetUniformLocationARB( program->object, "u_DepthTexture" );
//
//	locYUVTextureY = qglGetUniformLocationARB( program->object, "u_YUVTextureY" );
//	locYUVTextureU = qglGetUniformLocationARB( program->object, "u_YUVTextureU" );
//	locYUVTextureV = qglGetUniformLocationARB( program->object, "u_YUVTextureV" );
//
//	locColorLUT = qglGetUniformLocationARB( program->object, "u_ColorLUT" );
//
//	program->loc.DeluxemapOffset = qglGetUniformLocationARB( program->object, "u_DeluxemapOffset" );
//
//	for( i = 0; i < MAX_LIGHTMAPS; i++ ) {
//		// arrays of samplers are broken on ARM Mali so get u_LightmapTexture%i instead of u_LightmapTexture[%i]
//		locLightmapTexture[i] = qglGetUniformLocationARB( program->object, 
//			va_r( tmp, sizeof( tmp ), "u_LightmapTexture%i", i ) );
//
//		if( locLightmapTexture[i] < 0 )
//			break;
//
//		program->loc.LightstyleColor[i] = qglGetUniformLocationARB( program->object, 
//			va_r( tmp, sizeof( tmp ), "u_LightstyleColor[%i]", i ) );
//	}
//
//	program->loc.GlossFactors = qglGetUniformLocationARB( program->object, "u_GlossFactors" );
//
//	program->loc.OffsetMappingScale = qglGetUniformLocationARB( program->object, "u_OffsetMappingScale" );
//
//	program->loc.OutlineHeight = qglGetUniformLocationARB( program->object, "u_OutlineHeight" );
//	program->loc.OutlineCutOff = qglGetUniformLocationARB( program->object, "u_OutlineCutOff" );
//
//	program->loc.FrontPlane = qglGetUniformLocationARB( program->object, "u_FrontPlane" );
//
//	program->loc.TextureParams = qglGetUniformLocationARB( program->object, "u_TextureParams" );
//
//	program->loc.EntityDist = qglGetUniformLocationARB( program->object, "u_EntityDist" );
//	program->loc.EntityOrigin = qglGetUniformLocationARB( program->object, "u_EntityOrigin" );
//	program->loc.EntityColor = qglGetUniformLocationARB( program->object, "u_EntityColor" );
//	program->loc.ConstColor = qglGetUniformLocationARB( program->object, "u_ConstColor" );
//	program->loc.RGBGenFuncArgs = qglGetUniformLocationARB( program->object, "u_RGBGenFuncArgs" );
//	program->loc.AlphaGenFuncArgs = qglGetUniformLocationARB( program->object, "u_AlphaGenFuncArgs" );
//
//	program->loc.Fog.Plane = qglGetUniformLocationARB( program->object, "u_FogPlane" );
//	program->loc.Fog.Color = qglGetUniformLocationARB( program->object, "u_FogColor" );
//	program->loc.Fog.ScaleAndEyeDist = qglGetUniformLocationARB( program->object, "u_FogScaleAndEyeDist" );
//	program->loc.Fog.EyePlane = qglGetUniformLocationARB( program->object, "u_FogEyePlane" );
//
//	program->loc.ShaderTime = qglGetUniformLocationARB( program->object, "u_ShaderTime" );
//
//	program->loc.ReflectionTexMatrix = qglGetUniformLocationARB( program->object, "u_ReflectionTexMatrix" );
//	program->loc.VectorTexMatrix = qglGetUniformLocationARB( program->object, "u_VectorTexMatrix" );
//
//	program->loc.builtin.ViewOrigin = qglGetUniformLocationARB( program->object, "u_QF_ViewOrigin" );
//	program->loc.builtin.ViewAxis = qglGetUniformLocationARB( program->object, "u_QF_ViewAxis" );
//	program->loc.builtin.MirrorSide = qglGetUniformLocationARB( program->object, "u_QF_MirrorSide" );
//	program->loc.builtin.EntityOrigin = qglGetUniformLocationARB( program->object, "u_QF_EntityOrigin" );
//	program->loc.builtin.ShaderTime = qglGetUniformLocationARB( program->object, "u_QF_ShaderTime" );
//
//	// dynamic lights
//	for( i = 0; i < MAX_DLIGHTS; i++ ) {
//		program->loc.DynamicLightsPosition[i] = qglGetUniformLocationARB( program->object, 
//			va_r( tmp, sizeof( tmp ), "u_DlightPosition[%i]", i ) );
//
//		if( !( i & 3 ) ) {
//			// 4x4 transposed, so we can index it with `i`
//			program->loc.DynamicLightsDiffuseAndInvRadius[i >> 2] =
//				qglGetUniformLocationARB( program->object, va_r( tmp, sizeof( tmp ), "u_DlightDiffuseAndInvRadius[%i]", i ) );
//		}
//	}
//	program->loc.NumDynamicLights = qglGetUniformLocationARB( program->object, "u_NumDynamicLights" );
//
//	// shadowmaps
//	for( i = 0; i < GLSL_SHADOWMAP_LIMIT; i++ ) {
//		program->loc.ShadowmapTextureParams[i] = 
//			qglGetUniformLocationARB( program->object, va_r( tmp, sizeof( tmp ), "u_ShadowmapTextureParams[%i]", i ) );
//		if( program->loc.ShadowmapTextureParams[i] < 0 )
//			break;
//
//		program->loc.ShadowmapMatrix[i] = 
//			qglGetUniformLocationARB( program->object, va_r( tmp, sizeof( tmp ), "u_ShadowmapMatrix%i", i ) );
//
//		program->loc.ShadowDir[i] =
//			qglGetUniformLocationARB( program->object, va_r( tmp, sizeof( tmp ), "u_ShadowDir[%i]", i ) );
//
//		program->loc.ShadowEntityDist[i] =
//			qglGetUniformLocationARB( program->object, va_r( tmp, sizeof( tmp ), "u_ShadowEntityDist[%i]", i ) );
//
//		if( !( i & 3 ) ) {
//			program->loc.ShadowAlpha[i >> 2] =
//				qglGetUniformLocationARB( program->object, va_r( tmp, sizeof( tmp ), "u_ShadowAlpha[%i]", i >> 2 ) );
//		}
//	}
//
//	program->loc.BlendMix = qglGetUniformLocationARB( program->object, "u_BlendMix" );
//
//	program->loc.SoftParticlesScale = qglGetUniformLocationARB( program->object, "u_SoftParticlesScale" );
//
//	program->loc.DualQuats = qglGetUniformLocationARB( program->object, "u_DualQuats" );
//
//	program->loc.InstancePoints = qglGetUniformLocationARB( program->object, "u_InstancePoints" );
//	
//	program->loc.WallColor = qglGetUniformLocationARB( program->object, "u_WallColor" );
////	program->loc.FloorColor = qglGetUniformLocationARB( program->object, "u_FloorColor" );
//
//	if( locBaseTexture >= 0 )
//		qglUniform1iARB( locBaseTexture, 0 );
//	if( locDuDvMapTexture >= 0 )
//		qglUniform1iARB( locDuDvMapTexture, 0 );
//
//	if( locNormalmapTexture >= 0 )
//		qglUniform1iARB( locNormalmapTexture, 1 );
//	if( locGlossTexture >= 0 )
//		qglUniform1iARB( locGlossTexture, 2 );
//	if( locDecalTexture >= 0 )
//		qglUniform1iARB( locDecalTexture, 3 );
//	if( locEntityDecalTexture >= 0 )
//		qglUniform1iARB( locEntityDecalTexture, 4 );
//
//	if( locReflectionTexture >= 0 )
//		qglUniform1iARB( locReflectionTexture, 2 );
//	if( locRefractionTexture >= 0 )
//		qglUniform1iARB( locRefractionTexture, 3 );
//
//	for( i = 0; i < GLSL_SHADOWMAP_LIMIT && locShadowmapTexture[i] >= 0; i++ )
//		qglUniform1iARB( locShadowmapTexture[i], i );
//
////	if( locBaseTexture >= 0 )
////		qglUniform1iARB( locBaseTexture, 0 );
//	if( locCelShadeTexture >= 0 )
//		qglUniform1iARB( locCelShadeTexture, 1 );
//	if( locDiffuseTexture >= 0 )
//		qglUniform1iARB( locDiffuseTexture, 2 );
////	if( locDecalTexture >= 0 )
////		qglUniform1iARB( locDecalTexture, 3 );
////	if( locEntityDecalTexture >= 0 )
////		qglUniform1iARB( locEntityDecalTexture, 4 );
//	if( locStripesTexture >= 0 )
//		qglUniform1iARB( locStripesTexture, 5 );
//	if( locCelLightTexture >= 0 )
//		qglUniform1iARB( locCelLightTexture, 6 );
//
//	if( locDepthTexture >= 0 )
//		qglUniform1iARB( locDepthTexture, 3 );
//
//	for( i = 0; i < MAX_LIGHTMAPS && locLightmapTexture[i] >= 0; i++ )
//		qglUniform1iARB( locLightmapTexture[i], i+4 );
//
//	if( locYUVTextureY >= 0 )
//		qglUniform1iARB( locYUVTextureY, 0 );
//	if( locYUVTextureU >= 0 )
//		qglUniform1iARB( locYUVTextureU, 1 );
//	if( locYUVTextureV >= 0 )
//		qglUniform1iARB( locYUVTextureV, 2 );
//
//	if( locColorLUT >= 0 )
//		qglUniform1iARB( locColorLUT, 1 );
}

/*
* RP_BindAttrbibutesLocations
*/
static void RP_BindAttrbibutesLocations( struct glsl_program_s *program )
{
 // qglBindAttribLocationARB( program->object, VATTRIB_POSITION, "a_Position" ); 
 // qglBindAttribLocationARB( program->object, VATTRIB_SVECTOR, "a_SVector" ); 
 // qglBindAttribLocationARB( program->object, VATTRIB_NORMAL, "a_Normal" ); 
 // qglBindAttribLocationARB( program->object, VATTRIB_COLOR0, "a_Color" );
 // qglBindAttribLocationARB( program->object, VATTRIB_TEXCOORDS, "a_TexCoord" );

 // qglBindAttribLocationARB( program->object, VATTRIB_SPRITEPOINT, "a_SpritePoint" );
 // qglBindAttribLocationARB( program->object, VATTRIB_SVECTOR, "a_SpriteRightUpAxis" );

 // qglBindAttribLocationARB( program->object, VATTRIB_BONESINDICES, "a_BonesIndices" );
 // qglBindAttribLocationARB( program->object, VATTRIB_BONESWEIGHTS, "a_BonesWeights" );

 // qglBindAttribLocationARB( program->object, VATTRIB_LMCOORDS01, "a_LightmapCoord01" );
 // qglBindAttribLocationARB( program->object, VATTRIB_LMCOORDS23, "a_LightmapCoord23" );

 // if( glConfig.ext.texture_array ) {
 // 	qglBindAttribLocationARB( program->object, VATTRIB_LMLAYERS0123, "a_LightmapLayer0123" );
 // }

 // if( glConfig.ext.instanced_arrays ) {
 // 	qglBindAttribLocationARB( program->object, VATTRIB_INSTANCE_QUAT, "a_InstanceQuat" );
 // 	qglBindAttribLocationARB( program->object, VATTRIB_INSTANCE_XYZS, "a_InstancePosAndScale" );
 // }

//#ifndef GL_ES_VERSION_2_0
//	if( glConfig.shadingLanguageVersion >= 130 ) {
//		qglBindFragDataLocation( program->object, 0, "qf_FragColor" );
//	}
//#endif
}


/*
 * RP_Shutdown
 */
void RP_Shutdown( void )
{
	unsigned int i;
	struct glsl_program_s *program;

	qglUseProgram( 0 );

	for( i = 0, program = r_glslprograms; i < r_numglslprograms; i++, program++ ) {
		RF_DeleteProgram( program );
	}
	
	Trie_Destroy( glsl_cache_trie );
	glsl_cache_trie = NULL;

	r_numglslprograms = 0;
}
