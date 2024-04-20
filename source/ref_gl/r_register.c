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

// r_register.c
#include "r_local.h"
#include "../qalgo/hash.h"

#define STB_DS_IMPLEMENTATION 1
#include "stb_ds.h"

glconfig_t glConfig;

r_shared_t rsh;

mempool_t *r_mempool;

cvar_t *r_maxfps;
cvar_t *r_norefresh;
cvar_t *r_drawentities;
cvar_t *r_drawworld;
cvar_t *r_speeds;
cvar_t *r_drawelements;
cvar_t *r_fullbright;
cvar_t *r_lightmap;
cvar_t *r_novis;
cvar_t *r_nocull;
cvar_t *r_lerpmodels;
cvar_t *r_mapoverbrightbits;
cvar_t *r_brightness;

cvar_t *r_dynamiclight;
cvar_t *r_coronascale;
cvar_t *r_detailtextures;
cvar_t *r_subdivisions;
cvar_t *r_showtris;
cvar_t *r_draworder;
cvar_t *r_leafvis;

cvar_t *r_fastsky;
cvar_t *r_portalonly;
cvar_t *r_portalmaps;
cvar_t *r_portalmaps_maxtexsize;

cvar_t *r_lighting_deluxemapping;
cvar_t *r_lighting_specular;
cvar_t *r_lighting_glossintensity;
cvar_t *r_lighting_glossexponent;
cvar_t *r_lighting_ambientscale;
cvar_t *r_lighting_directedscale;
cvar_t *r_lighting_packlightmaps;
cvar_t *r_lighting_maxlmblocksize;
cvar_t *r_lighting_vertexlight;
cvar_t *r_lighting_maxglsldlights;
cvar_t *r_lighting_grayscale;

cvar_t *r_offsetmapping;
cvar_t *r_offsetmapping_scale;
cvar_t *r_offsetmapping_reliefmapping;

cvar_t *r_shadows;
cvar_t *r_shadows_alpha;
cvar_t *r_shadows_projection_distance;
cvar_t *r_shadows_maxtexsize;
cvar_t *r_shadows_pcf;
cvar_t *r_shadows_self_shadow;
cvar_t *r_shadows_dither;

cvar_t *r_outlines_world;
cvar_t *r_outlines_scale;
cvar_t *r_outlines_cutoff;

cvar_t *r_soft_particles;
cvar_t *r_soft_particles_scale;

cvar_t *r_fxaa;

cvar_t *r_lodbias;
cvar_t *r_lodscale;

cvar_t *r_stencilbits;
cvar_t *r_gamma;
cvar_t *r_texturebits;
cvar_t *r_texturemode;
cvar_t *r_texturefilter;
cvar_t *r_texturecompression;
cvar_t *r_picmip;
cvar_t *r_skymip;
cvar_t *r_nobind;
cvar_t *r_polyblend;
cvar_t *r_lockpvs;
cvar_t *r_screenshot_fmtstr;
cvar_t *r_screenshot_format;
cvar_t *r_swapinterval;
cvar_t *r_swapinterval_min;

cvar_t *r_temp1;

cvar_t *r_drawflat;
cvar_t *r_wallcolor;
cvar_t *r_floorcolor;

cvar_t *r_usenotexture;

cvar_t *r_maxglslbones;

cvar_t *gl_drawbuffer;
cvar_t *gl_driver;
cvar_t *gl_cull;
cvar_t *r_multithreading;

static bool	r_verbose;
static bool	r_postinit;

static void R_GfxInfo_f( void );

static void R_InitVolatileAssets( void );
static void R_DestroyVolatileAssets( void );

//=======================================================================

#define	GLINF_FOFS(x) offsetof(glextinfo_t,x)
#define	GLINF_EXMRK() GLINF_FOFS(_extMarker)
#define	GLINF_FROM(from,ofs) (*((char *)from + ofs))

typedef struct
{
	const char *name;				// constant pointer to constant string
	void **pointer;					// constant pointer to function's pointer (function itself)
} gl_extension_func_t;

typedef struct
{
	const char * prefix;			// constant pointer to constant string
	const char * name;
	const char * cvar_default;
	bool cvar_readonly;
	bool mandatory;
	gl_extension_func_t *funcs;		// constant pointer to array of functions
	size_t offset;					// offset to respective variable
	size_t depOffset;				// offset to required pre-initialized variable
} gl_extension_t;

#define GL_EXTENSION_FUNC_EXT(name,func) { name, (void ** const)func }
#define GL_EXTENSION_FUNC(name) GL_EXTENSION_FUNC_EXT("gl"#name,&(qgl##name))

#ifndef GL_ES_VERSION_2_0

/* GL_ARB_multitexture */
static const gl_extension_func_t gl_ext_multitexture_ARB_funcs[] =
{
	 GL_EXTENSION_FUNC(ActiveTextureARB)
	,GL_EXTENSION_FUNC(ClientActiveTextureARB)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_ARB_vertex_buffer_object */
static const gl_extension_func_t gl_ext_vertex_buffer_object_ARB_funcs[] =
{
	 GL_EXTENSION_FUNC(BindBufferARB)
	,GL_EXTENSION_FUNC(DeleteBuffersARB)
	,GL_EXTENSION_FUNC(GenBuffersARB)
	,GL_EXTENSION_FUNC(BufferDataARB)
	,GL_EXTENSION_FUNC(BufferSubDataARB)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_EXT_draw_range_elements */
static const gl_extension_func_t gl_ext_draw_range_elements_EXT_funcs[] =
{
	 GL_EXTENSION_FUNC(DrawRangeElementsEXT)
	,GL_EXTENSION_FUNC_EXT("glDrawRangeElements",&qglDrawRangeElementsEXT)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_ARB_GLSL (meta extension) */
static const gl_extension_func_t gl_ext_GLSL_ARB_funcs[] =
{
	 GL_EXTENSION_FUNC(DeleteObjectARB)
	,GL_EXTENSION_FUNC(DetachObjectARB)
	,GL_EXTENSION_FUNC(CreateShaderObjectARB)
	,GL_EXTENSION_FUNC(ShaderSourceARB)
	,GL_EXTENSION_FUNC(CompileShaderARB)
	,GL_EXTENSION_FUNC(CreateProgramObjectARB)
	,GL_EXTENSION_FUNC(AttachObjectARB)
	,GL_EXTENSION_FUNC(LinkProgramARB)
	,GL_EXTENSION_FUNC(UseProgramObjectARB)
	,GL_EXTENSION_FUNC(ValidateProgramARB)
	,GL_EXTENSION_FUNC(Uniform1fARB)
	,GL_EXTENSION_FUNC(Uniform2fARB)
	,GL_EXTENSION_FUNC(Uniform3fARB)
	,GL_EXTENSION_FUNC(Uniform4fARB)
	,GL_EXTENSION_FUNC(Uniform1iARB)
	,GL_EXTENSION_FUNC(Uniform2iARB)
	,GL_EXTENSION_FUNC(Uniform3iARB)
	,GL_EXTENSION_FUNC(Uniform4iARB)
	,GL_EXTENSION_FUNC(Uniform1fvARB)
	,GL_EXTENSION_FUNC(Uniform2fvARB)
	,GL_EXTENSION_FUNC(Uniform3fvARB)
	,GL_EXTENSION_FUNC(Uniform4fvARB)
	,GL_EXTENSION_FUNC(Uniform1ivARB)
	,GL_EXTENSION_FUNC(Uniform2ivARB)
	,GL_EXTENSION_FUNC(Uniform3ivARB)
	,GL_EXTENSION_FUNC(Uniform4ivARB)
	,GL_EXTENSION_FUNC(UniformMatrix2fvARB)
	,GL_EXTENSION_FUNC(UniformMatrix3fvARB)
	,GL_EXTENSION_FUNC(UniformMatrix4fvARB)
	,GL_EXTENSION_FUNC(GetObjectParameterivARB)
	,GL_EXTENSION_FUNC(GetInfoLogARB)
	,GL_EXTENSION_FUNC(GetAttachedObjectsARB)
	,GL_EXTENSION_FUNC(GetUniformLocationARB)
	,GL_EXTENSION_FUNC(GetActiveUniformARB)
	,GL_EXTENSION_FUNC(GetUniformfvARB)
	,GL_EXTENSION_FUNC(GetUniformivARB)
	,GL_EXTENSION_FUNC(GetShaderSourceARB)

	,GL_EXTENSION_FUNC(VertexAttribPointerARB)
	,GL_EXTENSION_FUNC(EnableVertexAttribArrayARB)
	,GL_EXTENSION_FUNC(DisableVertexAttribArrayARB)
	,GL_EXTENSION_FUNC(BindAttribLocationARB)
	,GL_EXTENSION_FUNC(GetActiveAttribARB)
	,GL_EXTENSION_FUNC(GetAttribLocationARB)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_ARB_GLSL_core (meta extension) */
static const gl_extension_func_t gl_ext_GLSL_core_ARB_funcs[] =
{
	 GL_EXTENSION_FUNC(DeleteProgram)
	,GL_EXTENSION_FUNC(DeleteShader)
	,GL_EXTENSION_FUNC(DetachShader)
	,GL_EXTENSION_FUNC(CreateShader)
	,GL_EXTENSION_FUNC(CreateProgram)
	,GL_EXTENSION_FUNC(AttachShader)
	,GL_EXTENSION_FUNC(UseProgram)
	,GL_EXTENSION_FUNC(GetProgramiv)
	,GL_EXTENSION_FUNC(GetShaderiv)
	,GL_EXTENSION_FUNC(GetProgramInfoLog)
	,GL_EXTENSION_FUNC(GetShaderInfoLog)
	,GL_EXTENSION_FUNC(GetAttachedShaders)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_ARB_GLSL130 (meta extension) */
static const gl_extension_func_t gl_ext_GLSL130_ARB_funcs[] =
{
	GL_EXTENSION_FUNC(BindFragDataLocation)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_ARB_draw_instanced */
static const gl_extension_func_t gl_ext_draw_instanced_ARB_funcs[] =
{
	 GL_EXTENSION_FUNC(DrawArraysInstancedARB)
	,GL_EXTENSION_FUNC(DrawElementsInstancedARB)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_ARB_instanced_arrays */
static const gl_extension_func_t gl_ext_instanced_arrays_ARB_funcs[] =
{
	 GL_EXTENSION_FUNC(VertexAttribDivisorARB)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_ARB_get_program_binary */
static const gl_extension_func_t gl_ext_get_program_binary_ARB_funcs[] =
{
	 GL_EXTENSION_FUNC(ProgramParameteri)
	,GL_EXTENSION_FUNC(GetProgramBinary)
	,GL_EXTENSION_FUNC(ProgramBinary)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

static const gl_extension_func_t gl_core_functions_32[] = { 
	GL_EXTENSION_FUNC( TexImage3D ),
	GL_EXTENSION_FUNC( TexSubImage3D ),

	GL_EXTENSION_FUNC( BindFramebuffer ),
	GL_EXTENSION_FUNC( BindAttribLocation ),

	GL_EXTENSION_FUNC( ShaderSource ),

	GL_EXTENSION_FUNC( DrawArraysInstanced ),
	GL_EXTENSION_FUNC( DrawElementsInstanced ),
	GL_EXTENSION_FUNC( VertexAttribDivisor ),

	GL_EXTENSION_FUNC( VertexAttribPointer ),
	GL_EXTENSION_FUNC( EnableVertexAttribArray ),
	GL_EXTENSION_FUNC( DisableVertexAttribArray ),
	GL_EXTENSION_FUNC( BindAttribLocation ),
	GL_EXTENSION_FUNC( GetActiveAttrib ),
	GL_EXTENSION_FUNC( GetAttribLocation ),

	GL_EXTENSION_FUNC( DeleteProgram ),
	GL_EXTENSION_FUNC( DeleteShader ),
	GL_EXTENSION_FUNC( DetachShader ),
	GL_EXTENSION_FUNC( CreateShader ),
	GL_EXTENSION_FUNC( ShaderSource ),
	GL_EXTENSION_FUNC( CompileShader ),
	GL_EXTENSION_FUNC( CreateProgram ),
	GL_EXTENSION_FUNC( AttachShader ),
	GL_EXTENSION_FUNC( LinkProgram ),
	GL_EXTENSION_FUNC( UseProgram ),
	GL_EXTENSION_FUNC( ValidateProgram ),
	GL_EXTENSION_FUNC( Uniform1f ),
	GL_EXTENSION_FUNC( Uniform2f ),
	GL_EXTENSION_FUNC( Uniform3f ),
	GL_EXTENSION_FUNC( Uniform4f ),
	GL_EXTENSION_FUNC( Uniform1i ),
	GL_EXTENSION_FUNC( Uniform2i ),
	GL_EXTENSION_FUNC( Uniform3i ),
	GL_EXTENSION_FUNC( Uniform4i ),
	GL_EXTENSION_FUNC( Uniform1fv ),
	GL_EXTENSION_FUNC( Uniform2fv ),
	GL_EXTENSION_FUNC( Uniform3fv ),
	GL_EXTENSION_FUNC( Uniform4fv ),
	GL_EXTENSION_FUNC( Uniform1iv ),
	GL_EXTENSION_FUNC( Uniform2iv ),
	GL_EXTENSION_FUNC( Uniform3iv ),
	GL_EXTENSION_FUNC( Uniform4iv ),
	GL_EXTENSION_FUNC( UniformMatrix2fv ),
	GL_EXTENSION_FUNC( UniformMatrix3fv ),
	GL_EXTENSION_FUNC( UniformMatrix4fv ),
	GL_EXTENSION_FUNC( GetProgramiv ),
	GL_EXTENSION_FUNC( GetShaderiv ),
	GL_EXTENSION_FUNC( GetProgramInfoLog ),
	GL_EXTENSION_FUNC( GetShaderInfoLog ),
	GL_EXTENSION_FUNC( GetAttachedShaders ),
	GL_EXTENSION_FUNC( GetUniformLocation ),
	GL_EXTENSION_FUNC( GetActiveUniform ),
	GL_EXTENSION_FUNC( GetUniformfv ),
	GL_EXTENSION_FUNC( GetUniformiv ),
	GL_EXTENSION_FUNC( GetShaderSource ),

	GL_EXTENSION_FUNC( BindFragDataLocation ),

	GL_EXTENSION_FUNC( GenFramebuffers ),
	
	GL_EXTENSION_FUNC( DebugMessageCallback ),

	GL_EXTENSION_FUNC( BindBuffer ),
	GL_EXTENSION_FUNC( DeleteBuffers ),
	GL_EXTENSION_FUNC( GenBuffers ),
	GL_EXTENSION_FUNC( BufferData ),
	GL_EXTENSION_FUNC( BufferSubData ),

	GL_EXTENSION_FUNC(IsRenderbuffer),
 	GL_EXTENSION_FUNC(BindRenderbuffer),
 	GL_EXTENSION_FUNC(DeleteRenderbuffers),
 	GL_EXTENSION_FUNC(GenRenderbuffers),
 	GL_EXTENSION_FUNC(RenderbufferStorage),
 	GL_EXTENSION_FUNC(GetRenderbufferParameteriv),
 	GL_EXTENSION_FUNC(IsFramebuffer),
 	GL_EXTENSION_FUNC(BindFramebuffer),
 	GL_EXTENSION_FUNC(DeleteFramebuffers),
 	GL_EXTENSION_FUNC(GenFramebuffers),
	GL_EXTENSION_FUNC(CheckFramebufferStatus),
	GL_EXTENSION_FUNC(FramebufferTexture2D),
	GL_EXTENSION_FUNC(FramebufferRenderbuffer),
	GL_EXTENSION_FUNC(GetFramebufferAttachmentParameteriv),
	GL_EXTENSION_FUNC(GenerateMipmap),
	
	GL_EXTENSION_FUNC(ActiveTexture),
	GL_EXTENSION_FUNC(ClientActiveTexture),

	GL_EXTENSION_FUNC(CompressedTexImage2D),
	GL_EXTENSION_FUNC(CompressedTexSubImage2D),

	GL_EXTENSION_FUNC_EXT( NULL, NULL ) 
};

/* GL_EXT_framebuffer_object */
static const gl_extension_func_t gl_ext_framebuffer_object_EXT_funcs[] =
{
	 GL_EXTENSION_FUNC(IsRenderbufferEXT)
 	,GL_EXTENSION_FUNC(BindRenderbufferEXT)
 	,GL_EXTENSION_FUNC(DeleteRenderbuffersEXT)
 	,GL_EXTENSION_FUNC(GenRenderbuffersEXT)
 	,GL_EXTENSION_FUNC(RenderbufferStorageEXT)
 	,GL_EXTENSION_FUNC(GetRenderbufferParameterivEXT)
 	,GL_EXTENSION_FUNC(IsFramebufferEXT)
 	,GL_EXTENSION_FUNC(BindFramebufferEXT)
 	,GL_EXTENSION_FUNC(DeleteFramebuffersEXT)
 	,GL_EXTENSION_FUNC(GenFramebuffersEXT)
	,GL_EXTENSION_FUNC(CheckFramebufferStatusEXT)
	,GL_EXTENSION_FUNC(FramebufferTexture1DEXT)
	,GL_EXTENSION_FUNC(FramebufferTexture2DEXT)
	,GL_EXTENSION_FUNC(FramebufferRenderbufferEXT)
	,GL_EXTENSION_FUNC(GetFramebufferAttachmentParameterivEXT)
	,GL_EXTENSION_FUNC(GenerateMipmapEXT)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_EXT_framebuffer_blit */
static const gl_extension_func_t gl_ext_framebuffer_blit_EXT_funcs[] =
{
	GL_EXTENSION_FUNC(BlitFramebufferEXT)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_ARB_texture_compression */
static const gl_extension_func_t gl_ext_texture_compression_ARB_funcs[] =
{
	 GL_EXTENSION_FUNC(CompressedTexImage2DARB)
	,GL_EXTENSION_FUNC(CompressedTexSubImage2DARB)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_EXT_blend_func_separate */
static const gl_extension_func_t gl_ext_blend_func_separate_EXT_funcs[] =
{
	 GL_EXTENSION_FUNC(BlendFuncSeparateEXT)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_EXT_texture3D */
static const gl_extension_func_t gl_ext_texture3D_EXT_funcs[] =
{
	 GL_EXTENSION_FUNC(TexImage3DEXT)
	,GL_EXTENSION_FUNC(TexSubImage3DEXT)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

#else // GL_ES_VERSION_2_0

/* GL_ANGLE_framebuffer_blit */
static const gl_extension_func_t gl_ext_framebuffer_blit_ANGLE_funcs[] =
{
	GL_EXTENSION_FUNC(BlitFramebufferANGLE)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_NV_framebuffer_blit */
static const gl_extension_func_t gl_ext_framebuffer_blit_NV_funcs[] =
{
	GL_EXTENSION_FUNC(BlitFramebufferNV)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_OES_get_program_binary */
static const gl_extension_func_t gl_ext_get_program_binary_OES_funcs[] =
{
	 GL_EXTENSION_FUNC(GetProgramBinaryOES)
	,GL_EXTENSION_FUNC(ProgramBinaryOES)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_OES_texture_3D */
static const gl_extension_func_t gl_ext_texture_3D_OES_funcs[] =
{
	 GL_EXTENSION_FUNC(TexImage3DOES)
	,GL_EXTENSION_FUNC(TexSubImage3DOES)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

#endif // GL_ES_VERSION_2_0

#ifndef USE_SDL2

#ifdef _WIN32

/* WGL_EXT_swap_interval */
static const gl_extension_func_t wgl_ext_swap_interval_EXT_funcs[] =
{
	 GL_EXTENSION_FUNC_EXT("wglSwapIntervalEXT",&qwglSwapIntervalEXT)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

#endif

#ifdef GLX_VERSION

/* GLX_SGI_swap_control */
static const gl_extension_func_t glx_ext_swap_control_SGI_funcs[] =
{
	 GL_EXTENSION_FUNC_EXT("glXSwapIntervalSGI",&qglXSwapIntervalSGI)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

#endif

#endif // USE_SDL2


static void R_RegisterFatalExt(const char* ext) {
	Sys_Error( "'%s' is not available, aborting\n", ext);
}

static bool R_TryLoadGLProcAddress(const gl_extension_func_t *funcs)
{
	const gl_extension_func_t *func = funcs;
	if( func ) {
		do {
			*( func->pointer ) = (void *)qglGetProcAddress( (const GLubyte *)func->name );
			if( !*( func->pointer ) ) {
				Com_Printf( "failed to load function: %s\n", func->name);
				break;
			}
		} while( ( ++func )->name );

		// some function is missing
		if( func->name ) {
			const gl_extension_func_t *func2 = funcs;
			// reset previously initialized functions back to NULL
			do {
				*( func2->pointer ) = NULL;
			} while( ( ++func2 )->name && func2 != func );
			return false;
		}
	}
	return true;
}

static void gl_callback(GLenum source,
            GLenum type,
            GLuint id,
            GLenum severity,
            GLsizei length,
            const char*msg,
            const void *userParam) {
 char* _source;
    char* _type;
	char *_severity;

	switch( source ) {
		case GL_DEBUG_SOURCE_API:
			_source = "API";
			break;

		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			_source = "WINDOW SYSTEM";
			break;

		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			_source = "SHADER COMPILER";
			break;

		case GL_DEBUG_SOURCE_THIRD_PARTY:
			_source = "THIRD PARTY";
			break;

		case GL_DEBUG_SOURCE_APPLICATION:
			_source = "APPLICATION";
			break;

		case GL_DEBUG_SOURCE_OTHER:
			_source = "UNKNOWN";
			break;

		default:
			_source = "UNKNOWN";
			break;
	}

	switch( type ) {
		case GL_DEBUG_TYPE_ERROR:
			_type = "ERROR";
			break;

		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			_type = "DEPRECATED BEHAVIOR";
			break;

		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			_type = "UDEFINED BEHAVIOR";
			break;

		case GL_DEBUG_TYPE_PORTABILITY:
			_type = "PORTABILITY";
			break;

		case GL_DEBUG_TYPE_PERFORMANCE:
			_type = "PERFORMANCE";
			break;

		case GL_DEBUG_TYPE_OTHER:
			_type = "OTHER";
			break;

		case GL_DEBUG_TYPE_MARKER:
			_type = "MARKER";
			break;

		default:
			_type = "UNKNOWN";
			break;
	}

	switch( severity ) {
		case GL_DEBUG_SEVERITY_HIGH:
			_severity = "HIGH";
			break;

		case GL_DEBUG_SEVERITY_MEDIUM:
			_severity = "MEDIUM";
			break;

		case GL_DEBUG_SEVERITY_LOW:
			_severity = "LOW";
			break;

		case GL_DEBUG_SEVERITY_NOTIFICATION:
			_severity = "NOTIFICATION";
			break;

		default:
			_severity = "UNKNOWN";
			break;
	}
	ri.Com_Printf("%d: %s of %s severity, raised from %s: %s\n",
            id, _type, _severity, _source, msg);
  if(severity == GL_DEBUG_SEVERITY_HIGH) {
  	assert(false);
  }
}

/*
* R_RegisterGLExtensions
*/
static bool R_RegisterGLExtensions( void )
{
	memset( &glConfig.ext, 0, sizeof( glextinfo_t ) );

 // if( R_TryLoadGLProcAddress( gl_ext_vertex_buffer_object_ARB_funcs ) ) {
 // 	glConfig.ext.vertex_buffer_object = 1;
 // } else {
 // 	R_RegisterFatalExt( "gl_ext_vertex_buffer_object_ARB_funcs " );
 // }

 // if( R_TryLoadGLProcAddress( gl_ext_framebuffer_object_EXT_funcs ) ) {
 // 	glConfig.ext.framebuffer_object = 1;
 // } else {
 // 	R_RegisterFatalExt( "gl_ext_framebuffer_object_EXT_funcs" );
 // }

 // if( R_TryLoadGLProcAddress( gl_ext_multitexture_ARB_funcs ) ) {
 // 	glConfig.ext.multitexture = 1;
 // 	glConfig.ext.vertex_shader = 1;
 // 	glConfig.ext.fragment_shader = 1;
 // 	glConfig.ext.shader_objects = 1;
 // 	glConfig.ext.shading_language_100 = 1;
 // } else {
 // 	R_RegisterFatalExt( "gl_ext_multitexture_ARB_funcs" );
 // }

 // if( R_TryLoadGLProcAddress( gl_ext_GLSL_ARB_funcs ) ) {
 // 	glConfig.ext.GLSL = 1;
 // } else {
 // 	R_RegisterFatalExt( "gl_ext_GLSL_ARB_funcs" );
 // }

 // if( R_TryLoadGLProcAddress( gl_ext_blend_func_separate_EXT_funcs ) ) {
 // 	glConfig.ext.blend_func_separate = 1;
 // } else {
 // 	R_RegisterFatalExt( "gl_ext_blend_func_separate_EXT_funcs" );
 // }

 // if( R_TryLoadGLProcAddress( gl_ext_GLSL_core_ARB_funcs ) ) {
 // 	glConfig.ext.GLSL_core = 1;
 // }

 // if( R_TryLoadGLProcAddress( gl_ext_GLSL130_ARB_funcs ) ) {
 // 	glConfig.ext.GLSL130 = 1;
 // }
 // 
 // if( R_TryLoadGLProcAddress( gl_ext_draw_range_elements_EXT_funcs ) ) {
 // 	glConfig.ext.draw_range_elements = 1;
 // }

 // if( R_TryLoadGLProcAddress( gl_ext_framebuffer_blit_EXT_funcs ) ) {
 // 	glConfig.ext.framebuffer_blit = 1;
 // }

 // if( R_TryLoadGLProcAddress( gl_ext_texture_compression_ARB_funcs ) ) {
 // 	glConfig.ext.texture_compression = 1;
 // }

 // if( R_TryLoadGLProcAddress( gl_ext_draw_instanced_ARB_funcs ) ) {
 // 	glConfig.ext.draw_instanced = 1;
 // }
 // if( R_TryLoadGLProcAddress( gl_ext_instanced_arrays_ARB_funcs ) ) {
 // 	glConfig.ext.instanced_arrays = 1;
 // }

 // if( R_TryLoadGLProcAddress( gl_ext_get_program_binary_ARB_funcs ) ) {
 // 	glConfig.ext.get_program_binary = 1;
 // }

 // if( R_TryLoadGLProcAddress( gl_ext_texture3D_EXT_funcs ) ) {
 // 	glConfig.ext.texture3D = 1;
 // 	glConfig.ext.texture_array = 1;
 // }

	if( R_TryLoadGLProcAddress( gl_core_functions_32 ) ) {
		qglDebugMessageCallback(gl_callback, NULL);


		qglTexImage3DEXT = qglTexImage3D;
		qglTexSubImage3DEXT = qglTexSubImage3D;
		qglVertexAttribPointerARB = qglVertexAttribPointer;
		qglEnableVertexAttribArrayARB = qglEnableVertexAttribArray;
		qglDisableVertexAttribArrayARB = qglDisableVertexAttribArray;
		qglBindAttribLocationARB = qglBindAttribLocation;
		qglGetActiveAttribARB = qglGetActiveAttrib;
		qglGetAttribLocationARB = qglGetAttribLocation;

		qglBindFramebufferEXT = qglBindFramebuffer;
		qglShaderSourceARB = qglShaderSource;
	
		qglDrawArraysInstancedARB = qglDrawArraysInstanced;
		qglDrawElementsInstancedARB = qglDrawElementsInstanced;
		qglVertexAttribDivisorARB = qglVertexAttribDivisor;

		qglCompileShaderARB = qglCompileShader;

		qglDeleteObjectARB = qglDeleteProgram;
		// qglDeleteShader = qglDeleteShaderARB;
		// qglDetachShader = qglDetachShaderARB;
		// qglCreateShader = qglCreateShaderARB;
		qglShaderSourceARB = qglShaderSource;
		qglCompileShaderARB = qglCompileShader;
		// qglCreateProgram = qglCreateProgramARB;
		// qglAttachShader = qglAttachShaderARB;
		qglLinkProgramARB = qglLinkProgram;
		// qglUseProgram = qglUseProgramARB;
		qglValidateProgramARB = qglValidateProgram;
		qglUniform1fARB = qglUniform1f;
		qglUniform2fARB = qglUniform2f;
		qglUniform3fARB = qglUniform3f;
		qglUniform4fARB = qglUniform4f;
		qglUniform1iARB = qglUniform1i;
		qglUniform2iARB = qglUniform2i;
		qglUniform3iARB = qglUniform3i;
		qglUniform4iARB = qglUniform4i;
		qglUniform1fvARB = qglUniform1fv;
		qglUniform2fvARB = qglUniform2fv;
		qglUniform3fvARB = qglUniform3fv;
		qglUniform4fvARB = qglUniform4fv;
		qglUniform1ivARB = qglUniform1iv;
		qglUniform2ivARB = qglUniform2iv;
		qglUniform3ivARB = qglUniform3iv;
		qglUniform4ivARB = qglUniform4iv;
		qglUniformMatrix2fvARB = qglUniformMatrix2fv;
		qglUniformMatrix3fvARB = qglUniformMatrix3fv;
		qglUniformMatrix4fvARB = qglUniformMatrix4fv;
		// qglGetProgramiv = qglGetProgramivARB;
		// qglGetShaderiv = qglGetShaderivARB;
		// qglGetProgramInfoLog = qglGetProgramInfoLogARB;
		// qglGetShaderInfoLog = qglGetShaderInfoLogARB;
		// qglGetAttachedShaders = qglGetAttachedShadersARB;
		qglGetUniformLocationARB = qglGetUniformLocation;
		qglGetActiveUniformARB = qglGetActiveUniform;
		qglGetUniformfvARB = qglGetUniformfv;
		qglGetUniformivARB = qglGetUniformiv;
		qglGetShaderSourceARB = qglGetShaderSource;

		qglGenFramebuffersEXT = qglGenFramebuffers;
		qglBufferDataARB = qglBufferData;
		qglBindBufferARB = qglBindBuffer;
		qglDeleteBuffersARB = qglDeleteBuffers;
		qglGenBuffersARB = qglGenBuffers;
		qglBufferSubDataARB = qglBufferSubData;
		
		qglFramebufferTexture2DEXT = qglFramebufferTexture2D;

		qglIsRenderbufferEXT = qglIsRenderbuffer;
		qglBindRenderbufferEXT = qglBindRenderbuffer;
		qglDeleteRenderbuffersEXT = qglDeleteRenderbuffers;
		qglGenRenderbuffersEXT = qglGenRenderbuffers;
		qglRenderbufferStorageEXT = qglRenderbufferStorage;
		qglGetRenderbufferParameterivEXT = qglGetRenderbufferParameteriv;
		qglIsFramebufferEXT = qglIsFramebuffer;
		qglBindFramebufferEXT = qglBindFramebuffer;
		qglDeleteFramebuffersEXT = qglDeleteFramebuffers;
		qglGenFramebuffersEXT = qglGenFramebuffers;
		qglCheckFramebufferStatusEXT = qglCheckFramebufferStatus;
		qglFramebufferTexture2DEXT = qglFramebufferTexture2D;
		qglFramebufferRenderbufferEXT = qglFramebufferRenderbuffer;
		qglGetFramebufferAttachmentParameterivEXT = qglGetFramebufferAttachmentParameteriv;
		qglGenerateMipmapEXT = qglGenerateMipmap;

		qglActiveTextureARB = qglActiveTexture; 
		qglClientActiveTextureARB = qglActiveTexture;

		qglCompressedTexImage2DARB = qglCompressedTexImage2D;
		qglCompressedTexSubImage2DARB = qglCompressedTexSubImage2D;

		glConfig.ext.GLSL130 = 1;
		glConfig.ext.blend_func_separate = 1;
		glConfig.ext.multitexture = 1;
		glConfig.ext.vertex_shader = 1;
		glConfig.ext.fragment_shader = 1;
		glConfig.ext.shader_objects = 1;
		glConfig.ext.shading_language_100 = 1;
		glConfig.ext.framebuffer_object = 1;
 		glConfig.ext.texture3D = 1;
  	glConfig.ext.texture_array = 1;
  	glConfig.ext.GLSL_core = 1;
  	glConfig.ext.GLSL = 1;
  	glConfig.ext.draw_range_elements = 1;
  	glConfig.ext.framebuffer_blit = 1;
  	glConfig.ext.texture_compression = 1;
    glConfig.ext.vertex_buffer_object = 1;
  } else {
  	R_RegisterFatalExt( "gl_ext_multitexture_ARB_funcs" );
  }
	glConfig.ext.bgra = 1;
	glConfig.ext.texture_filter_anisotropic = 1;
	glConfig.ext.meminfo = 1;
	glConfig.ext.gpu_memory_info = 1;
	glConfig.ext.gpu_shader5 = 1;
	glConfig.ext.texture_lod = 1;
	glConfig.ext.packed_depth_stencil = 1;
	glConfig.ext.ES3_compatibility = 1;
	glConfig.ext.half_float_vertex = 1;
	glConfig.ext.texture_edge_clamp = 1;
	glConfig.ext.texture_cube_map = 1;
	glConfig.ext.depth_texture = 1;
	glConfig.ext.shadow = 1;
	glConfig.ext.texture_non_power_of_two = 1;

#ifndef USE_SDL2
#ifdef GLX_VERSION
	if( R_TryLoadGLProcAddress( glx_ext_swap_control_SGI_funcs ) ) {
		glConfig.ext.swap_control = 1;
	}
#endif
#ifdef _WIN32
	if( R_TryLoadGLProcAddress( wgl_ext_swap_interval_EXT_funcs ) ) {
		glConfig.ext.swap_control = 1;
	}
#endif
#endif

	{
		char tmp[128];
		int versionMajor = 0;
		int versionMinor = 0;
		sscanf( glConfig.versionString, "%d.%d", &versionMajor, &versionMinor );
		glConfig.version = versionMajor * 100 + versionMinor * 10;

		glConfig.ext.depth24 = true;
		glConfig.ext.fragment_precision_high = true;
		glConfig.ext.rgb8_rgba8 = true;

		glConfig.maxTextureSize = 0;
		qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize );
		if( glConfig.maxTextureSize <= 0 )
			glConfig.maxTextureSize = 256;
		glConfig.maxTextureSize = 1 << Q_log2( glConfig.maxTextureSize );

		ri.Cvar_Get( "gl_max_texture_size", "0", CVAR_READONLY );
		ri.Cvar_ForceSet( "gl_max_texture_size", va_r( tmp, sizeof( tmp ), "%i", glConfig.maxTextureSize ) );

		/* GL_ARB_GLSL_core (meta extension) */
		if( !glConfig.ext.GLSL_core ) {
			qglDeleteProgram = qglDeleteObjectARB;
			qglDeleteShader = qglDeleteObjectARB;
			qglDetachShader = qglDetachObjectARB;
			qglCreateShader = qglCreateShaderObjectARB;
			qglCreateProgram = qglCreateProgramObjectARB;
			qglAttachShader = qglAttachObjectARB;
			qglUseProgram = qglUseProgramObjectARB;
			qglGetProgramiv = qglGetObjectParameterivARB;
			qglGetShaderiv = qglGetObjectParameterivARB;
			qglGetProgramInfoLog = qglGetInfoLogARB;
			qglGetShaderInfoLog = qglGetInfoLogARB;
			qglGetAttachedShaders = qglGetAttachedObjectsARB;
		}

		/* GL_ARB_texture_cube_map */
		glConfig.maxTextureCubemapSize = 0;
		qglGetIntegerv( GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig.maxTextureCubemapSize );
		glConfig.maxTextureCubemapSize = 1 << Q_log2( glConfig.maxTextureCubemapSize );

		/* GL_ARB_multitexture */
		glConfig.maxTextureUnits = 1;
		qglGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &glConfig.maxTextureUnits );
		clamp( glConfig.maxTextureUnits, 1, MAX_TEXTURE_UNITS );

		/* GL_EXT_framebuffer_object */
		glConfig.maxRenderbufferSize = 0;
		qglGetIntegerv( GL_MAX_RENDERBUFFER_SIZE_EXT, &glConfig.maxRenderbufferSize );
		glConfig.maxRenderbufferSize = 1 << Q_log2( glConfig.maxRenderbufferSize );
		if( glConfig.maxRenderbufferSize > glConfig.maxTextureSize )
			glConfig.maxRenderbufferSize = glConfig.maxTextureSize;

		/* GL_EXT_texture_filter_anisotropic */
		glConfig.maxTextureFilterAnisotropic = 0;
		if( strstr( glConfig.extensionsString, "GL_EXT_texture_filter_anisotropic" ) )
			qglGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureFilterAnisotropic );

		/* GL_EXT_texture3D and GL_EXT_texture_array */
		glConfig.maxTexture3DSize = 0;
		glConfig.maxTextureLayers = 0;
		if( glConfig.ext.texture3D )
			qglGetIntegerv( GL_MAX_3D_TEXTURE_SIZE_EXT, &glConfig.maxTexture3DSize );
		if( glConfig.ext.texture_array )
			qglGetIntegerv( GL_MAX_ARRAY_TEXTURE_LAYERS_EXT, &glConfig.maxTextureLayers );
		/* GL_EXT_packed_depth_stencil
		 * Many OpenGL implementation don't support separate depth and stencil renderbuffers. */
		if( !glConfig.ext.packed_depth_stencil )
			glConfig.stencilBits = 0;

		versionMajor = versionMinor = 0;
		sscanf( glConfig.shadingLanguageVersionString, "%d.%d", &versionMajor, &versionMinor );
		glConfig.shadingLanguageVersion = versionMajor * 100 + versionMinor;
		if( !glConfig.ext.GLSL130 ) {
			glConfig.shadingLanguageVersion = 120;
		}

		glConfig.maxVertexUniformComponents = glConfig.maxFragmentUniformComponents = 0;
		glConfig.maxVaryingFloats = 0;

		qglGetIntegerv( GL_MAX_VERTEX_ATTRIBS_ARB, &glConfig.maxVertexAttribs );
		qglGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &glConfig.maxVertexUniformComponents );
		qglGetIntegerv( GL_MAX_VARYING_FLOATS_ARB, &glConfig.maxVaryingFloats );
		qglGetIntegerv( GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB, &glConfig.maxFragmentUniformComponents );

		// instance attributes are beyond the minimum number of attributes supported by GLES2
		if( glConfig.maxVertexAttribs <= VATTRIB_INSTANCE_XYZS ) {
			glConfig.ext.instanced_arrays = false;
		}

		// keep the maximum number of bones we can do in GLSL sane
		if( r_maxglslbones->integer > MAX_GLSL_UNIFORM_BONES ) {
			ri.Cvar_ForceSet( r_maxglslbones->name, r_maxglslbones->dvalue );
		}

		// require GLSL 1.20+ for GPU skinning
		if( glConfig.shadingLanguageVersion >= 120 ) {
			// the maximum amount of bones we can handle in a vertex shader (2 vec4 uniforms per vertex)
			glConfig.maxGLSLBones = bound( 0, glConfig.maxVertexUniformComponents / 8 - 19, r_maxglslbones->integer );
		} else {
			glConfig.maxGLSLBones = 0;
		}

		if( glConfig.ext.texture_non_power_of_two ) {
			// blacklist this extension on Radeon X1600-X1950 hardware (they support it only with certain filtering/repeat modes)
			int val = 0;

			// LadyHavoc: this is blocked on Mac OS X because the drivers claim support but often can't accelerate it or crash when used.
#ifndef __APPLE__
			if( glConfig.ext.vertex_shader )
				qglGetIntegerv( GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB, &val );
#endif

			if( val <= 0 )
				glConfig.ext.texture_non_power_of_two = false;
		}

		if( glConfig.ext.depth24 ) {
			glConfig.depthEpsilon = 1.0 / ( 1 << 22 );
		} else {
			glConfig.depthEpsilon = 1.0 / ( 1 << 14 );
		}

		cvar_t *cvar = ri.Cvar_Get( "gl_ext_vertex_buffer_object_hack", "0", CVAR_ARCHIVE | CVAR_NOSET );
		if( cvar && !cvar->integer ) {
			ri.Cvar_ForceSet( cvar->name, "1" );
			ri.Cvar_ForceSet( "gl_ext_vertex_buffer_object", "1" );
		}

		ri.Cvar_Get( "r_texturefilter_max", "0", CVAR_READONLY );
		ri.Cvar_ForceSet( "r_texturefilter_max", va_r( tmp, sizeof( tmp ), "%i", glConfig.maxTextureFilterAnisotropic ) );

		ri.Cvar_Get( "r_soft_particles_available", "0", CVAR_READONLY );
		if( glConfig.ext.depth_texture && glConfig.ext.fragment_precision_high && glConfig.ext.framebuffer_blit )
			ri.Cvar_ForceSet( "r_soft_particles_available", "1" );

		// don't allow too high values for lightmap block size as they negatively impact performance
		if( r_lighting_maxlmblocksize->integer > glConfig.maxTextureSize / 4 && !( glConfig.maxTextureSize / 4 < min( QF_LIGHTMAP_WIDTH, QF_LIGHTMAP_HEIGHT ) * 2 ) )
			ri.Cvar_ForceSet( "r_lighting_maxlmblocksize", va_r( tmp, sizeof( tmp ), "%i", glConfig.maxTextureSize / 4 ) );
	}
	return true;
}

static void R_PrintGLExtensionsString( const char *name, const char *str )
{
	size_t len, p;

	Com_Printf( "%s: ", name );

	if( str && *str )
	{
		for( len = strlen( str ), p = 0; p < len;  )
		{
			char chunk[512];

			Q_snprintfz( chunk, sizeof( chunk ), "%s", str + p );
			p += strlen( chunk );
			
			Com_Printf( "%s", chunk );
		}
	}
	else
	{
		Com_Printf( "none" );
	}

	Com_Printf( "\n" );
}

/*
* R_PrintMemoryInfo
*/
static void R_PrintMemoryInfo( void )
{
	int mem[12];

	Com_Printf( "\n" );
	Com_Printf( "Video memory information:\n" );

	if( glConfig.ext.gpu_memory_info ) {
		// NV
		qglGetIntegerv( GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, mem );
		Com_Printf( "total: %i MB\n", mem[0] >>10 );

		qglGetIntegerv( GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, mem );
		Com_Printf( "dedicated: %i MB\n", mem[0] >>10 );

		qglGetIntegerv( GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, mem );
		Com_Printf( "available: %i MB\n", mem[0] >>10 );

		qglGetIntegerv( GPU_MEMORY_INFO_EVICTION_COUNT_NVX, mem );
		Com_Printf( "eviction count: %i MB\n", mem[0] >> 10 );

		qglGetIntegerv( GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, mem );
		Com_Printf( "totally evicted: %i MB\n", mem[0] >>10 );
	}
	else if( glConfig.ext.meminfo ) {
		// ATI
		qglGetIntegerv( VBO_FREE_MEMORY_ATI, mem );
		qglGetIntegerv( TEXTURE_FREE_MEMORY_ATI, mem+4 );
		qglGetIntegerv( RENDERBUFFER_FREE_MEMORY_ATI, mem+8 );

		Com_Printf( "total memory free in the pool: (VBO:%i, Tex:%i, RBuf:%i) MB\n", mem[0] >> 10, mem[4] >> 10, mem[8] >> 10 );
		Com_Printf( "largest available free block in the pool: (V:%i, Tex:%i, RBuf:%i) MB\n", mem[5] >> 10, mem[4] >> 10, mem[9] >> 10 );
		Com_Printf( "total auxiliary memory free: (VBO:%i, Tex:%i, RBuf:%i) MB\n", mem[2] >> 10, mem[6] >> 10, mem[10] >> 10 );
		Com_Printf( "largest auxiliary free block: (VBO:%i, Tex:%i, RBuf:%i) MB\n", mem[3] >> 10, mem[7] >> 10, mem[11] >> 10 );
	}
	else {
		Com_Printf( "not available\n" );
	}
}


/*
* R_FillStartupBackgroundColor
*
* Fills the window with a color during the initialization.
*/
static void R_FillStartupBackgroundColor( float r, float g, float b )
{
	qglClearColor( r, g, b, 1.0 );
	GLimp_BeginFrame();
#ifndef GL_ES_VERSION_2_0
	if( glConfig.stereoEnabled )
	{
		qglDrawBuffer( GL_BACK_LEFT );
		qglClear( GL_COLOR_BUFFER_BIT );
		qglDrawBuffer( GL_BACK_RIGHT );
		qglClear( GL_COLOR_BUFFER_BIT );
		qglDrawBuffer( GL_BACK );
	}
#endif
	qglClear( GL_COLOR_BUFFER_BIT );
	qglFinish();
	GLimp_EndFrame();
}

static void R_Register( const char *screenshotsPrefix )
{
	char tmp[128];
	const qgl_driverinfo_t *driver;

    r_maxfps = ri.Cvar_Get( "r_maxfps", "250", CVAR_ARCHIVE );
	r_norefresh = ri.Cvar_Get( "r_norefresh", "0", 0 );
	r_fullbright = ri.Cvar_Get( "r_fullbright", "0", CVAR_LATCH_VIDEO );
	r_lightmap = ri.Cvar_Get( "r_lightmap", "0", 0 );
	r_drawentities = ri.Cvar_Get( "r_drawentities", "1", CVAR_CHEAT );
	r_drawworld = ri.Cvar_Get( "r_drawworld", "1", CVAR_CHEAT );
	r_novis = ri.Cvar_Get( "r_novis", "0", 0 );
	r_nocull = ri.Cvar_Get( "r_nocull", "0", 0 );
	r_lerpmodels = ri.Cvar_Get( "r_lerpmodels", "1", 0 );
	r_speeds = ri.Cvar_Get( "r_speeds", "0", 0 );
	r_drawelements = ri.Cvar_Get( "r_drawelements", "1", 0 );
	r_showtris = ri.Cvar_Get( "r_showtris", "0", CVAR_CHEAT );
	r_leafvis = ri.Cvar_Get( "r_leafvis", "0", CVAR_CHEAT );
	r_lockpvs = ri.Cvar_Get( "r_lockpvs", "0", CVAR_CHEAT );
	r_nobind = ri.Cvar_Get( "r_nobind", "0", 0 );
	r_picmip = ri.Cvar_Get( "r_picmip", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_skymip = ri.Cvar_Get( "r_skymip", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_polyblend = ri.Cvar_Get( "r_polyblend", "1", 0 );

	r_mapoverbrightbits = ri.Cvar_Get( "r_mapoverbrightbits", "2", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_brightness = ri.Cvar_Get( "r_brightness", "0", CVAR_ARCHIVE );

	r_detailtextures = ri.Cvar_Get( "r_detailtextures", "1", CVAR_ARCHIVE );

	r_dynamiclight = ri.Cvar_Get( "r_dynamiclight", "1", CVAR_ARCHIVE );
	r_coronascale = ri.Cvar_Get( "r_coronascale", "0.4", 0 );
	r_subdivisions = ri.Cvar_Get( "r_subdivisions", STR_TOSTR( SUBDIVISIONS_DEFAULT ), CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_draworder = ri.Cvar_Get( "r_draworder", "0", CVAR_CHEAT );

	r_fastsky = ri.Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE );
	r_portalonly = ri.Cvar_Get( "r_portalonly", "0", 0 );
	r_portalmaps = ri.Cvar_Get( "r_portalmaps", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_portalmaps_maxtexsize = ri.Cvar_Get( "r_portalmaps_maxtexsize", "1024", CVAR_ARCHIVE );

	r_lighting_deluxemapping = ri.Cvar_Get( "r_lighting_deluxemapping", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_specular = ri.Cvar_Get( "r_lighting_specular", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_glossintensity = ri.Cvar_Get( "r_lighting_glossintensity", "1.5", CVAR_ARCHIVE );
	r_lighting_glossexponent = ri.Cvar_Get( "r_lighting_glossexponent", "24", CVAR_ARCHIVE );
	r_lighting_ambientscale = ri.Cvar_Get( "r_lighting_ambientscale", "1", 0 );
	r_lighting_directedscale = ri.Cvar_Get( "r_lighting_directedscale", "1", 0 );

	r_lighting_packlightmaps = ri.Cvar_Get( "r_lighting_packlightmaps", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_maxlmblocksize = ri.Cvar_Get( "r_lighting_maxlmblocksize", "2048", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_vertexlight = ri.Cvar_Get( "r_lighting_vertexlight", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_maxglsldlights = ri.Cvar_Get( "r_lighting_maxglsldlights", "16", CVAR_ARCHIVE );
	r_lighting_grayscale = ri.Cvar_Get( "r_lighting_grayscale", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );

	r_offsetmapping = ri.Cvar_Get( "r_offsetmapping", "2", CVAR_ARCHIVE );
	r_offsetmapping_scale = ri.Cvar_Get( "r_offsetmapping_scale", "0.02", CVAR_ARCHIVE );
	r_offsetmapping_reliefmapping = ri.Cvar_Get( "r_offsetmapping_reliefmapping", "0", CVAR_ARCHIVE );

#ifdef CGAMEGETLIGHTORIGIN
	r_shadows = ri.Cvar_Get( "cg_shadows", "1", CVAR_ARCHIVE );
#else
	r_shadows = ri.Cvar_Get( "r_shadows", "0", CVAR_ARCHIVE );
#endif
	r_shadows_alpha = ri.Cvar_Get( "r_shadows_alpha", "0.7", CVAR_ARCHIVE );
	r_shadows_projection_distance = ri.Cvar_Get( "r_shadows_projection_distance", "64", CVAR_CHEAT );
	r_shadows_maxtexsize = ri.Cvar_Get( "r_shadows_maxtexsize", "64", CVAR_ARCHIVE );
	r_shadows_pcf = ri.Cvar_Get( "r_shadows_pcf", "1", CVAR_ARCHIVE );
	r_shadows_self_shadow = ri.Cvar_Get( "r_shadows_self_shadow", "0", CVAR_ARCHIVE );
	r_shadows_dither = ri.Cvar_Get( "r_shadows_dither", "0", CVAR_ARCHIVE );

	r_outlines_world = ri.Cvar_Get( "r_outlines_world", "1.8", CVAR_ARCHIVE );
	r_outlines_scale = ri.Cvar_Get( "r_outlines_scale", "1", CVAR_ARCHIVE );
	r_outlines_cutoff = ri.Cvar_Get( "r_outlines_cutoff", "712", CVAR_ARCHIVE );

	r_soft_particles = ri.Cvar_Get( "r_soft_particles", "1", CVAR_ARCHIVE );
	r_soft_particles_scale = ri.Cvar_Get( "r_soft_particles_scale", "0.02", CVAR_ARCHIVE );

	r_fxaa = ri.Cvar_Get( "r_fxaa", "1", CVAR_ARCHIVE );

	r_lodbias = ri.Cvar_Get( "r_lodbias", "0", CVAR_ARCHIVE );
	r_lodscale = ri.Cvar_Get( "r_lodscale", "5.0", CVAR_ARCHIVE );

	r_gamma = ri.Cvar_Get( "r_gamma", "1.0", CVAR_ARCHIVE );
	r_texturebits = ri.Cvar_Get( "r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH_VIDEO );
	r_texturemode = ri.Cvar_Get( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
	r_texturefilter = ri.Cvar_Get( "r_texturefilter", "4", CVAR_ARCHIVE );
	r_texturecompression = ri.Cvar_Get( "r_texturecompression", "0", CVAR_ARCHIVE | CVAR_LATCH_VIDEO );
	r_stencilbits = ri.Cvar_Get( "r_stencilbits", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );

	r_screenshot_fmtstr = ri.Cvar_Get( "r_screenshot_fmtstr", va_r( tmp, sizeof( tmp ), "%s%y%%m%%d_%H%M%%S", screenshotsPrefix ), CVAR_ARCHIVE );
	r_screenshot_format = ri.Cvar_Get( "r_screenshot_format", "1", CVAR_ARCHIVE );

#if defined(GLX_VERSION) && !defined(USE_SDL2)
	r_swapinterval = ri.Cvar_Get( "r_swapinterval", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
#else
	r_swapinterval = ri.Cvar_Get( "r_swapinterval", "0", CVAR_ARCHIVE );
#endif
	r_swapinterval_min = ri.Cvar_Get( "r_swapinterval_min", "0", CVAR_READONLY ); // exposes vsync support to UI

	r_temp1 = ri.Cvar_Get( "r_temp1", "0", 0 );

	r_drawflat = ri.Cvar_Get( "r_drawflat", "0", CVAR_ARCHIVE );
	r_wallcolor = ri.Cvar_Get( "r_wallcolor", "255 255 255", CVAR_ARCHIVE );
	r_floorcolor = ri.Cvar_Get( "r_floorcolor", "255 153 0", CVAR_ARCHIVE );

	// make sure we rebuild our 3D texture after vid_restart
	r_wallcolor->modified = r_floorcolor->modified = true;

	r_maxfps->modified = true;

	// set to 1 to enable use of the checkerboard texture for missing world and model images
	r_usenotexture = ri.Cvar_Get( "r_usenotexture", "0", CVAR_ARCHIVE );

	r_maxglslbones = ri.Cvar_Get( "r_maxglslbones", STR_TOSTR( MAX_GLSL_UNIFORM_BONES ), CVAR_LATCH_VIDEO );

	r_multithreading = ri.Cvar_Get( "r_multithreading", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );

	gl_cull = ri.Cvar_Get( "gl_cull", "1", 0 );
	gl_drawbuffer = ri.Cvar_Get( "gl_drawbuffer", "GL_BACK", 0 );

	driver = QGL_GetDriverInfo();
	if( driver && driver->dllcvarname )
		gl_driver = ri.Cvar_Get( driver->dllcvarname, driver->dllname, CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	else
		gl_driver = NULL;

	ri.Cmd_AddCommand( "imagelist", R_ImageList_f );
	ri.Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	ri.Cmd_AddCommand( "shaderdump", R_ShaderDump_f );
	ri.Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	ri.Cmd_AddCommand( "envshot", R_EnvShot_f );
	ri.Cmd_AddCommand( "modellist", Mod_Modellist_f );
	ri.Cmd_AddCommand( "gfxinfo", R_GfxInfo_f );
	ri.Cmd_AddCommand( "glslprogramlist", RP_ProgramList_f );
	ri.Cmd_AddCommand( "cinlist", R_CinList_f );
}

/*
* R_GfxInfo_f
*/
static void R_GfxInfo_f( void )
{
	Com_Printf( "\n" );
	Com_Printf( "GL_VENDOR: %s\n", glConfig.vendorString );
	Com_Printf( "GL_RENDERER: %s\n", glConfig.rendererString );
	Com_Printf( "GL_VERSION: %s\n", glConfig.versionString );
	Com_Printf( "GL_SHADING_LANGUAGE_VERSION: %s\n", glConfig.shadingLanguageVersionString );

	R_PrintGLExtensionsString( "GL_EXTENSIONS", glConfig.extensionsString );
	R_PrintGLExtensionsString( "GLXW_EXTENSIONS", glConfig.glwExtensionsString );

	Com_Printf( "GL_MAX_TEXTURE_SIZE: %i\n", glConfig.maxTextureSize );
	Com_Printf( "GL_MAX_TEXTURE_IMAGE_UNITS: %i\n", glConfig.maxTextureUnits );
	Com_Printf( "GL_MAX_CUBE_MAP_TEXTURE_SIZE: %i\n", glConfig.maxTextureCubemapSize );
	if( glConfig.ext.texture3D )
		Com_Printf( "GL_MAX_3D_TEXTURE_SIZE: %i\n", glConfig.maxTexture3DSize );
	if( glConfig.ext.texture_array )
		Com_Printf( "GL_MAX_ARRAY_TEXTURE_LAYERS: %i\n", glConfig.maxTextureLayers );
	if( glConfig.ext.texture_filter_anisotropic )
		Com_Printf( "GL_MAX_TEXTURE_MAX_ANISOTROPY: %i\n", glConfig.maxTextureFilterAnisotropic );
	Com_Printf( "GL_MAX_RENDERBUFFER_SIZE: %i\n", glConfig.maxRenderbufferSize );
	Com_Printf( "GL_MAX_VARYING_FLOATS: %i\n", glConfig.maxVaryingFloats );
	Com_Printf( "GL_MAX_VERTEX_UNIFORM_COMPONENTS: %i\n", glConfig.maxVertexUniformComponents );
	Com_Printf( "GL_MAX_VERTEX_ATTRIBS: %i\n", glConfig.maxVertexAttribs );
	Com_Printf( "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS: %i\n", glConfig.maxFragmentUniformComponents );
	Com_Printf( "\n" );

	Com_Printf( "mode: %ix%i%s\n", glConfig.width, glConfig.height,
		glConfig.fullScreen ? ", fullscreen" : ", windowed" );
	Com_Printf( "picmip: %i\n", r_picmip->integer );
	Com_Printf( "texturemode: %s\n", r_texturemode->string );
	Com_Printf( "anisotropic filtering: %i\n", r_texturefilter->integer );
	Com_Printf( "vertical sync: %s\n", ( r_swapinterval->integer || r_swapinterval_min->integer ) ? "enabled" : "disabled" );
	Com_Printf( "multithreading: %s\n", glConfig.multithreading ? "enabled" : "disabled" );

	R_PrintMemoryInfo();
}

/*
* R_GLVersionHash
*/
static unsigned R_GLVersionHash( const char *vendorString, 
	const char *rendererString, const char *versionString )
{
	uint8_t *tmp;
	size_t csize;
	size_t tmp_size, pos;
	unsigned hash;

	tmp_size = strlen( vendorString ) + strlen( rendererString ) +
		strlen( versionString ) + strlen( ARCH ) + 1;

	pos = 0;
	tmp = R_Malloc( tmp_size );

	csize = strlen( vendorString );
	memcpy( tmp + pos, vendorString, csize );
	pos += csize;

	csize = strlen( rendererString );
	memcpy( tmp + pos, rendererString, csize );
	pos += csize;

	csize = strlen( versionString );
	memcpy( tmp + pos, versionString, csize );
	pos += csize;

	// shaders are not compatible between 32-bit and 64-bit at least on Nvidia
	csize = strlen( ARCH );
	memcpy( tmp + pos, ARCH, csize );
	pos += csize;

	hash = COM_SuperFastHash( tmp, tmp_size, tmp_size );

	R_Free( tmp );

	return hash;
}

/*
* R_Init
*/
rserr_t R_Init( const char *applicationName, const char *screenshotPrefix, int startupColor,
	int iconResource, const int *iconXPM,
	void *hinstance, void *wndproc, void *parenthWnd, 
	bool verbose )
{
	qgl_initerr_t initerr;

	r_mempool = R_AllocPool( NULL, "Rendering Frontend" );
	r_verbose = verbose;
	r_postinit = true;

	if( !applicationName ) applicationName = "Qfusion";
	if( !screenshotPrefix ) screenshotPrefix = "";

	R_Register( screenshotPrefix );

	memset( &glConfig, 0, sizeof(glConfig) );

	// initialize our QGL dynamic bindings
	const char *dllname = NULL;
	const qgl_driverinfo_t *driver = QGL_GetDriverInfo();
	if( driver ) {
		dllname = driver->dllname;
	}
init_qgl:
	initerr = QGL_Init( gl_driver ? gl_driver->string : dllname );
	if( initerr != qgl_initerr_ok )
	{
		QGL_Shutdown();
		Com_Printf( "ref_gl::R_Init() - could not load \"%s\"\n", gl_driver ? gl_driver->string : dllname );

		if( ( initerr == qgl_initerr_invalid_driver ) && gl_driver && strcmp( gl_driver->string, dllname ) )
		{
			ri.Cvar_ForceSet( gl_driver->name, dllname );
			goto init_qgl;
		}

		return rserr_invalid_driver;
	}


	// initialize OS-specific parts of OpenGL
	if( !GLimp_Init( applicationName, hinstance, wndproc, parenthWnd, iconResource, iconXPM ) )
	{
		QGL_Shutdown();
		return rserr_unknown;
	}

	// FIXME: move this elsewhere?
	glConfig.applicationName = R_CopyString( applicationName );
	glConfig.screenshotPrefix = R_CopyString( screenshotPrefix );
	glConfig.startupColor = startupColor;

	return rserr_ok;
}

/*
* R_PostInit
*/
static rserr_t R_PostInit( void )
{
	GLenum glerr;

	glConfig.hwGamma = GLimp_GetGammaRamp( GAMMARAMP_STRIDE, &glConfig.gammaRampSize, glConfig.originalGammaRamp );
	if( glConfig.hwGamma )
		r_gamma->modified = true;

	/*
	** get our various GL strings
	*/
	glConfig.vendorString = (const char *)qglGetString( GL_VENDOR );
	glConfig.rendererString = (const char *)qglGetString( GL_RENDERER );
	glConfig.versionString = (const char *)qglGetString( GL_VERSION );
	glConfig.extensionsString = (const char *)qglGetString( GL_EXTENSIONS );
	glConfig.glwExtensionsString = (const char *)qglGetGLWExtensionsString ();
	glConfig.shadingLanguageVersionString = (const char *)qglGetString( GL_SHADING_LANGUAGE_VERSION_ARB );

	if( !glConfig.vendorString ) glConfig.vendorString = "";
	if( !glConfig.rendererString ) glConfig.rendererString = "";
	if( !glConfig.versionString ) glConfig.versionString = "";
	if( !glConfig.extensionsString ) glConfig.extensionsString = "";
	if( !glConfig.glwExtensionsString ) glConfig.glwExtensionsString = "";
	if( !glConfig.shadingLanguageVersionString ) glConfig.shadingLanguageVersionString = "";

	glConfig.versionHash = R_GLVersionHash( glConfig.vendorString, glConfig.rendererString,
		glConfig.versionString );
	glConfig.multithreading = r_multithreading->integer != 0 && !strstr( glConfig.vendorString, "nouveau" );

	memset( &rsh, 0, sizeof( rsh ) );
	memset( &rf, 0, sizeof( rf ) );

	rsh.registrationSequence = 1;
	rsh.registrationOpen = false;

	rsh.worldModelSequence = 1;

	rf.swapInterval = -1;
	rf.speedsMsgLock = ri.Mutex_Create();
	rf.debugSurfaceLock = ri.Mutex_Create();

	R_InitDrawLists();

	if( !R_RegisterGLExtensions() ) {
		QGL_Shutdown();
		return rserr_unknown;
	}

	R_SetSwapInterval( 0, -1 );

    R_FillStartupBackgroundColor( COLOR_R( glConfig.startupColor ) / 255.0f,
		COLOR_G( glConfig.startupColor ) / 255.0f, COLOR_B( glConfig.startupColor ) / 255.0f );

	R_TextureMode( r_texturemode->string );

	R_AnisotropicFilter( r_texturefilter->integer );

	if ( r_verbose )
		R_GfxInfo_f();

	// load and compile GLSL programs
	RP_Init();

	R_InitVBO();

	R_InitImages();

	R_InitShaders();

	R_InitCinematics();

	R_InitSkinFiles();

	R_InitModels();

	R_ClearScene();

	R_InitVolatileAssets();

	R_ClearRefInstStack();

	glerr = qglGetError();
	if( glerr != GL_NO_ERROR )
		Com_Printf( "glGetError() = 0x%x\n", glerr );

	return rserr_ok;
}

/*
* R_SetMode
*/
rserr_t R_SetMode( int x, int y, int width, int height, int displayFrequency, bool fullScreen, bool stereo )
{
	rserr_t err;
	
	err = GLimp_SetMode( x, y, width, height, displayFrequency, fullScreen, stereo );
	if( err != rserr_ok )
	{
		Com_Printf( "Could not GLimp_SetMode()\n" );
		return err;
	}

	if( r_postinit )
	{
		err = R_PostInit();
		r_postinit = false;
	}

	return err;
}

/*
* R_InitVolatileAssets
*/
static void R_InitVolatileAssets( void )
{
	// init volatile data
	R_InitSkeletalCache();
	R_InitCoronas();
	R_InitCustomColors();

	rsh.envShader = R_LoadShader( "$environment", SHADER_TYPE_OPAQUE_ENV, true );
	rsh.skyShader = R_LoadShader( "$skybox", SHADER_TYPE_SKYBOX, true );
	rsh.whiteShader = R_LoadShader( "$whiteimage", SHADER_TYPE_2D, true );
	rsh.emptyFogShader = R_LoadShader( "$emptyfog", SHADER_TYPE_FOG, true );

	if( !rsh.nullVBO ) {
		rsh.nullVBO = R_InitNullModelVBO();
	}
	else {
		R_TouchMeshVBO( rsh.nullVBO );
	}

	if( !rsh.postProcessingVBO ) {
		rsh.postProcessingVBO = R_InitPostProcessingVBO();
	}
	else {
		R_TouchMeshVBO( rsh.postProcessingVBO );
	}
}

/*
* R_DestroyVolatileAssets
*/
static void R_DestroyVolatileAssets( void )
{
	// kill volatile data
	R_ShutdownCustomColors();
	R_ShutdownCoronas();
	R_ShutdownSkeletalCache();
}

/*
* R_BeginRegistration
*/
void R_BeginRegistration( void )
{
	R_FinishLoadingImages();

	R_DestroyVolatileAssets();

	rsh.registrationSequence++;
	if( !rsh.registrationSequence ) {
		// make sure assumption that an asset is free it its registrationSequence is 0
		// since rsh.registrationSequence never equals 0
		rsh.registrationSequence = 1; 
	}
	rsh.registrationOpen = true;

	R_InitVolatileAssets();

	R_DeferDataSync();

	R_DataSync();
}

/*
* R_EndRegistration
*/
void R_EndRegistration( void )
{
	if( rsh.registrationOpen == false ) {
		return;
	}

	rsh.registrationOpen = false;

	R_FreeUnusedModels();
	R_FreeUnusedVBOs();
	R_FreeUnusedSkinFiles();
	R_FreeUnusedShaders();
	R_FreeUnusedCinematics();
	R_FreeUnusedImages();

	R_RestartCinematics();

	R_DeferDataSync();

	R_DataSync();
}

/*
* R_Shutdown
*/
void R_Shutdown( bool verbose )
{
	ri.Cmd_RemoveCommand( "modellist" );
	ri.Cmd_RemoveCommand( "screenshot" );
	ri.Cmd_RemoveCommand( "envshot" );
	ri.Cmd_RemoveCommand( "imagelist" );
	ri.Cmd_RemoveCommand( "gfxinfo" );
	ri.Cmd_RemoveCommand( "shaderdump" );
	ri.Cmd_RemoveCommand( "shaderlist" );
	ri.Cmd_RemoveCommand( "glslprogramlist" );
	ri.Cmd_RemoveCommand( "cinlist" );

	// free shaders, models, etc.

	R_DestroyVolatileAssets();

	R_ShutdownModels();

	R_ShutdownSkinFiles();

	R_ShutdownVBO();

	R_ShutdownShaders();

	R_ShutdownCinematics();

	R_ShutdownImages();

	// destroy compiled GLSL programs
	RP_Shutdown();

	// restore original gamma
	if( glConfig.hwGamma )
		GLimp_SetGammaRamp( GAMMARAMP_STRIDE, glConfig.gammaRampSize, glConfig.originalGammaRamp );

    ri.Mutex_Destroy( &rf.speedsMsgLock );
	ri.Mutex_Destroy( &rf.debugSurfaceLock );

	// shut down OS specific OpenGL stuff like contexts, etc.
	GLimp_Shutdown();

	// shutdown our QGL subsystem
	QGL_Shutdown();

	R_FreePool( &r_mempool );
}
