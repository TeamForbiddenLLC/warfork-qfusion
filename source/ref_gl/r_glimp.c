#include "r_glimp.h"
#include "r_local.h"
#include "../qalgo/hash.h"


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

#undef GL_EXTENSION_FUNC
#undef GL_EXTENSION_FUNC_EXT

//=======================================================================

#define GL_EXTENSION_EXT(pre,name,val,ro,mnd,funcs,dep) { #pre, #name, #val, ro, mnd, (gl_extension_func_t * const)funcs, GLINF_FOFS(name), GLINF_FOFS(dep) }
#define GL_EXTENSION(pre,name,ro,mnd,funcs) GL_EXTENSION_EXT(pre,name,1,ro,mnd,funcs,_extMarker)

//
// OpenGL extensions list
//
// short notation: vendor, name, default value, list of functions
// extended notation: vendor, name, default value, list of functions, required extension
static const gl_extension_t gl_extensions_decl[] =
{
#ifndef GL_ES_VERSION_2_0
	// extensions required by meta-extension gl_ext_GLSL
	 GL_EXTENSION( ARB, multitexture, true, true, &gl_ext_multitexture_ARB_funcs )
	,GL_EXTENSION( ARB, vertex_buffer_object, true, true, &gl_ext_vertex_buffer_object_ARB_funcs )
	,GL_EXTENSION_EXT( ARB, vertex_shader, 1, true, true, NULL, multitexture )
	,GL_EXTENSION_EXT( ARB, fragment_shader, 1, true, true, NULL, vertex_shader )
	,GL_EXTENSION_EXT( ARB, shader_objects, 1, true,true,  NULL, fragment_shader )
	,GL_EXTENSION_EXT( ARB, shading_language_100, 1, true, true, NULL, shader_objects )

	// meta GLSL extensions
	,GL_EXTENSION_EXT( \0, GLSL, 1, true, true, &gl_ext_GLSL_ARB_funcs, shading_language_100 )
	,GL_EXTENSION_EXT( \0, GLSL_core, 1, true, false, &gl_ext_GLSL_core_ARB_funcs, GLSL )
	,GL_EXTENSION_EXT( \0, GLSL130, 1, false, false, &gl_ext_GLSL130_ARB_funcs, GLSL )

	,GL_EXTENSION( EXT, draw_range_elements, false, false, &gl_ext_draw_range_elements_EXT_funcs )
	,GL_EXTENSION( EXT, framebuffer_object, true, true, &gl_ext_framebuffer_object_EXT_funcs )
	,GL_EXTENSION_EXT( EXT, framebuffer_blit, 1, false, false, &gl_ext_framebuffer_blit_EXT_funcs, framebuffer_object )
	,GL_EXTENSION( ARB, texture_compression, false, false, &gl_ext_texture_compression_ARB_funcs )
	,GL_EXTENSION( EXT, texture_edge_clamp, true, true, NULL )
	,GL_EXTENSION( SGIS, texture_edge_clamp, true, true, NULL )
	,GL_EXTENSION( ARB, texture_cube_map, true, true, NULL )
	,GL_EXTENSION( ARB, depth_texture, false, false, NULL )
	,GL_EXTENSION( SGIX, depth_texture, false, false, NULL )
	,GL_EXTENSION_EXT( ARB, shadow, 1, false, false, NULL, depth_texture )
	,GL_EXTENSION( ARB, texture_non_power_of_two, false, false, NULL )
	,GL_EXTENSION( ARB, draw_instanced, false, false, &gl_ext_draw_instanced_ARB_funcs )
	,GL_EXTENSION( ARB, instanced_arrays, false, false, &gl_ext_instanced_arrays_ARB_funcs )
	,GL_EXTENSION( ARB, half_float_vertex, false, false, NULL )
	,GL_EXTENSION( ARB, get_program_binary, false, false, &gl_ext_get_program_binary_ARB_funcs )
	,GL_EXTENSION( ARB, ES3_compatibility, false, false, NULL )
	,GL_EXTENSION( EXT, blend_func_separate, true, true, &gl_ext_blend_func_separate_EXT_funcs )
	,GL_EXTENSION( EXT, texture3D, false, false, &gl_ext_texture3D_EXT_funcs )
	,GL_EXTENSION_EXT( EXT, texture_array, 1, false, false, NULL, texture3D )
	,GL_EXTENSION( EXT, packed_depth_stencil, false, false, NULL )
	,GL_EXTENSION( SGIS, texture_lod, false, false, NULL )
	,GL_EXTENSION( ARB, gpu_shader5, false, false, NULL )

	// memory info
	,GL_EXTENSION( NVX, gpu_memory_info, true, false, NULL )
	,GL_EXTENSION( ATI, meminfo, true, false, NULL )

#else
	 GL_EXTENSION( NV, framebuffer_blit, false, false, &gl_ext_framebuffer_blit_NV_funcs )
	,GL_EXTENSION( ANGLE, framebuffer_blit, false, false, &gl_ext_framebuffer_blit_ANGLE_funcs )
	,GL_EXTENSION( OES, depth_texture, false, false, NULL )
	,GL_EXTENSION_EXT( EXT, shadow_samplers, 1, false, false, NULL, depth_texture )
	,GL_EXTENSION( OES, texture_npot, false, false, NULL )
	,GL_EXTENSION( OES, vertex_half_float, false, false, NULL )
	,GL_EXTENSION( OES, get_program_binary, false, false, &gl_ext_get_program_binary_OES_funcs )
	,GL_EXTENSION( OES, depth24, false, false, NULL )
	,GL_EXTENSION( NV, depth_nonlinear, false, false, NULL )
	,GL_EXTENSION( OES, rgb8_rgba8, true, false, NULL )
	,GL_EXTENSION( OES, texture_3D, false, false, &gl_ext_texture_3D_OES_funcs )
	,GL_EXTENSION( EXT, texture_array, false, false, &gl_ext_texture_3D_OES_funcs )
	,GL_EXTENSION( OES, compressed_ETC1_RGB8_texture, false, false, NULL )
	// Require depth24 because Tegra 3 doesn't support non-linear packed depth.
	,GL_EXTENSION_EXT( OES, packed_depth_stencil, 1, false, false, NULL, depth24 )
	,GL_EXTENSION( EXT, gpu_shader5, false, false, NULL )
#endif

	,GL_EXTENSION( EXT, texture_filter_anisotropic, true, false, NULL )
	,GL_EXTENSION( EXT, bgra, true, false, NULL )

#ifndef USE_SDL2
#ifdef GLX_VERSION
	,GL_EXTENSION( GLX_SGI, swap_control, true, false, &glx_ext_swap_control_SGI_funcs )
#endif
#ifdef _WIN32
	,GL_EXTENSION( WGL_EXT, swap_control, true, false, &wgl_ext_swap_interval_EXT_funcs )
#endif
#endif
};

static const int num_gl_extensions = sizeof( gl_extensions_decl ) / sizeof( gl_extensions_decl[0] );

#undef GL_EXTENSION
#undef GL_EXTENSION_EXT


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

bool GLimp_InitConfig() {
	int i;
	char *var, name[128];
	cvar_t *cvar;
	cvar_flag_t cvar_flags;
	const gl_extension_t *extension;
	int versionMajor = 0;
	int versionMinor = 0;
	int val;
	char tmp[128];

	glConfig.hwGamma = GLimp_GetGammaRamp( GAMMARAMP_STRIDE, &glConfig.gammaRampSize, glConfig.originalGammaRamp );
	if( glConfig.hwGamma )
		r_gamma->modified = true;

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

	for( i = 0, extension = gl_extensions_decl; i < num_gl_extensions; i++, extension++ )
	{
		Q_snprintfz( name, sizeof( name ), "gl_ext_%s", extension->name );

		// register a cvar and check if this extension is explicitly disabled
		cvar_flags = CVAR_ARCHIVE|CVAR_LATCH_VIDEO;
#ifdef PUBLIC_BUILD
		if( extension->cvar_readonly ) {
			cvar_flags |= CVAR_READONLY;
		}
#endif

		cvar = ri.Cvar_Get( name, extension->cvar_default ? extension->cvar_default : "0", cvar_flags );
		if( !cvar->integer )
			continue;

		// an alternative extension of higher priority is available so ignore this one
		var = &(GLINF_FROM( &glConfig.ext, extension->offset ));
		if( *var )
			continue;

		// required extension is not available, ignore
		if( extension->depOffset != GLINF_EXMRK() && !GLINF_FROM( &glConfig.ext, extension->depOffset ) ) 
			continue;

		// let's see what the driver's got to say about this...
		if( *extension->prefix )
		{
			const char *extstring = ( !strncmp( extension->prefix, "WGL", 3 ) ||
				!strncmp( extension->prefix, "GLX", 3 ) ||
				!strncmp( extension->prefix, "EGL", 3 ) ) ? glConfig.glwExtensionsString : glConfig.extensionsString;

			Q_snprintfz( name, sizeof( name ), "%s_%s", extension->prefix, extension->name );
			if( !strstr( extstring, name ) )
				continue;
		}

		// initialize function pointers
		gl_extension_func_t *func = extension->funcs;
		if( func )
		{
			do {
				*(func->pointer) = ( void * )qglGetProcAddress( (const GLubyte *)func->name );
				if( !*(func->pointer) )
					break;
			} while( (++func)->name );

			// some function is missing
			if( func->name )
			{
				gl_extension_func_t *func2 = extension->funcs;

				// whine about buggy driver
				if( *extension->prefix ) {
					Com_Printf( "R_RegisterGLExtensions: broken %s support, contact your video card vendor\n", 
						cvar->name );
				}

				// reset previously initialized functions back to NULL
				do {
					*(func2->pointer) = NULL;
				} while( (++func2)->name && func2 != func );

				continue;
			}
		}

		// mark extension as available
		*var = true;

	}

	for( i = 0, extension = gl_extensions_decl; i < num_gl_extensions; i++, extension++ )
	{
		if( !extension->mandatory ) {
			continue;
		}
		
		var = &(GLINF_FROM( &glConfig.ext, extension->offset ));

		if( !*var ) {
			Sys_Error( "R_RegisterGLExtensions: '%s_%s' is not available, aborting\n", 
				extension->prefix, extension->name );
			return false;
		}
	}

#ifdef GL_ES_VERSION_2_0
	sscanf( glConfig.versionString, "OpenGL ES %d.%d", &versionMajor, &versionMinor );
#else
	sscanf( glConfig.versionString, "%d.%d", &versionMajor, &versionMinor );
#endif
	glConfig.version = versionMajor * 100 + versionMinor * 10;

#ifdef GL_ES_VERSION_2_0
	glConfig.ext.multitexture = true;
	glConfig.ext.vertex_buffer_object = true;
	glConfig.ext.framebuffer_object = true;
	glConfig.ext.texture_compression = true;
	glConfig.ext.texture_edge_clamp = true;
	glConfig.ext.texture_cube_map = true;
	glConfig.ext.vertex_shader = true;
	glConfig.ext.fragment_shader = true;
	glConfig.ext.shader_objects = true;
	glConfig.ext.shading_language_100 = true;
	glConfig.ext.GLSL = true;
	glConfig.ext.GLSL_core = true;
	glConfig.ext.blend_func_separate = true;
	if( glConfig.version >= 300 )
	{
#define GL_OPTIONAL_CORE_EXTENSION(name) \
	( glConfig.ext.name = ( ri.Cvar_Get( "gl_ext_" #name, "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO )->integer ? true : false ) )
#define GL_OPTIONAL_CORE_EXTENSION_DEP(name,dep) \
	( glConfig.ext.name = ( ( glConfig.ext.dep && ri.Cvar_Get( "gl_ext_" #name, "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO )->integer ) ? true : false ) )
		glConfig.ext.ES3_compatibility = true;
		glConfig.ext.GLSL130 = true;
		glConfig.ext.rgb8_rgba8 = true;
		GL_OPTIONAL_CORE_EXTENSION(depth24);
		GL_OPTIONAL_CORE_EXTENSION(depth_texture);
		GL_OPTIONAL_CORE_EXTENSION(draw_instanced);
		GL_OPTIONAL_CORE_EXTENSION(draw_range_elements);
		GL_OPTIONAL_CORE_EXTENSION(framebuffer_blit);
		GL_OPTIONAL_CORE_EXTENSION(get_program_binary);
		GL_OPTIONAL_CORE_EXTENSION(instanced_arrays);
		GL_OPTIONAL_CORE_EXTENSION(texture_3D);
		GL_OPTIONAL_CORE_EXTENSION(texture_array);
		GL_OPTIONAL_CORE_EXTENSION(texture_lod);
		GL_OPTIONAL_CORE_EXTENSION(texture_npot);
		GL_OPTIONAL_CORE_EXTENSION(vertex_half_float);
		GL_OPTIONAL_CORE_EXTENSION_DEP(packed_depth_stencil, depth24);
		GL_OPTIONAL_CORE_EXTENSION_DEP(shadow_samplers, depth_texture);
#undef GL_OPTIONAL_CORE_EXTENSION_DEP
#undef GL_OPTIONAL_CORE_EXTENSION
	}
#else // GL_ES_VERSION_2_0
	glConfig.ext.depth24 = true;
	glConfig.ext.fragment_precision_high = true;
	glConfig.ext.rgb8_rgba8 = true;
#endif

	glConfig.maxTextureSize = 0;
	qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize );
	if( glConfig.maxTextureSize <= 0 )
		glConfig.maxTextureSize = 256;
	glConfig.maxTextureSize = 1 << Q_log2( glConfig.maxTextureSize );

	ri.Cvar_Get( "gl_max_texture_size", "0", CVAR_READONLY );
	ri.Cvar_ForceSet( "gl_max_texture_size", va_r( tmp, sizeof( tmp ), "%i", glConfig.maxTextureSize ) );

	/* GL_ARB_GLSL_core (meta extension) */
#ifndef GL_ES_VERSION_2_0
	if( !glConfig.ext.GLSL_core )
	{
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
#endif

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

	/* GL_EXT_framebuffer_blit */
#ifdef GL_ES_VERSION_2_0
	if( glConfig.ext.framebuffer_blit && !qglBlitFramebufferEXT )
	{
		if( qglBlitFramebufferNV )
			qglBlitFramebufferEXT = qglBlitFramebufferNV;
		else if( qglBlitFramebufferANGLE )
			qglBlitFramebufferEXT = qglBlitFramebufferANGLE;
		else
			glConfig.ext.framebuffer_blit = false;
	}
#endif

	/* GL_ARB_get_program_binary */
#ifdef GL_ES_VERSION_2_0
	if( glConfig.version < 300 )
	{
		qglGetProgramBinary = qglGetProgramBinaryOES;
		qglProgramBinary = qglProgramBinaryOES;
	}
#endif

	/* GL_EXT_texture3D and GL_EXT_texture_array */
	glConfig.maxTexture3DSize = 0;
	glConfig.maxTextureLayers = 0;
	if( glConfig.ext.texture3D )
		qglGetIntegerv( GL_MAX_3D_TEXTURE_SIZE_EXT, &glConfig.maxTexture3DSize );
	if( glConfig.ext.texture_array )
		qglGetIntegerv( GL_MAX_ARRAY_TEXTURE_LAYERS_EXT, &glConfig.maxTextureLayers );
#ifdef GL_ES_VERSION_2_0
	if( glConfig.version >= 300 )
	{
		qglTexImage3DEXT = qglTexImage3D;
		qglTexSubImage3DEXT = qglTexSubImage3D;
	}
#endif

	/* GL_OES_fragment_precision_high
	 * This extension has been withdrawn and some drivers don't expose it anymore,
	 * so it's not on the list and is activated here instead. */
#ifdef GL_ES_VERSION_2_0
	if( ri.Cvar_Get( "gl_ext_fragment_precision_high", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO )->integer )
	{
		int range[2] = { 0 }, precision = 0;
		qglGetShaderPrecisionFormat( GL_FRAGMENT_SHADER_ARB, GL_HIGH_FLOAT, range, &precision );
		if( range[0] && range[1] && precision )
			glConfig.ext.fragment_precision_high = true;
	}
#endif

	/* GL_EXT_packed_depth_stencil
	 * Many OpenGL implementation don't support separate depth and stencil renderbuffers. */
	if( !glConfig.ext.packed_depth_stencil )
		glConfig.stencilBits = 0;

	versionMajor = versionMinor = 0;
#ifdef GL_ES_VERSION_2_0
	sscanf( glConfig.shadingLanguageVersionString, "OpenGL ES GLSL ES %d.%d", &versionMajor, &versionMinor );
	if( !versionMajor )
		sscanf( glConfig.shadingLanguageVersionString, "OpenGL ES GLSL %d.%d", &versionMajor, &versionMinor );
#else
	sscanf( glConfig.shadingLanguageVersionString, "%d.%d", &versionMajor, &versionMinor );
#endif
	glConfig.shadingLanguageVersion = versionMajor * 100 + versionMinor;
#ifndef GL_ES_VERSION_2_0
	if( !glConfig.ext.GLSL130 ) {
		glConfig.shadingLanguageVersion = 120;
	}
#endif

	glConfig.maxVertexUniformComponents = glConfig.maxFragmentUniformComponents = 0;
	glConfig.maxVaryingFloats = 0;

	qglGetIntegerv( GL_MAX_VERTEX_ATTRIBS_ARB, &glConfig.maxVertexAttribs );
#ifdef GL_ES_VERSION_2_0
	qglGetIntegerv( GL_MAX_VERTEX_UNIFORM_VECTORS, &glConfig.maxVertexUniformComponents );
	qglGetIntegerv( GL_MAX_VARYING_VECTORS, &glConfig.maxVaryingFloats );
	qglGetIntegerv( GL_MAX_FRAGMENT_UNIFORM_VECTORS, &glConfig.maxFragmentUniformComponents );
	glConfig.maxVertexUniformComponents *= 4;
	glConfig.maxVaryingFloats *= 4;
	glConfig.maxFragmentUniformComponents *= 4;
#else
	qglGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &glConfig.maxVertexUniformComponents );
	qglGetIntegerv( GL_MAX_VARYING_FLOATS_ARB, &glConfig.maxVaryingFloats );
	qglGetIntegerv( GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB, &glConfig.maxFragmentUniformComponents );
#endif

	// instance attributes are beyond the minimum number of attributes supported by GLES2
	if( glConfig.maxVertexAttribs <= VATTRIB_INSTANCE_XYZS ) {
		glConfig.ext.instanced_arrays = false;
	}

	// keep the maximum number of bones we can do in GLSL sane
	if( r_maxglslbones->integer > MAX_GLSL_UNIFORM_BONES ) {
		ri.Cvar_ForceSet( r_maxglslbones->name, r_maxglslbones->dvalue );
	}

#ifdef GL_ES_VERSION_2_0
	glConfig.maxGLSLBones = bound( 0, glConfig.maxVertexUniformComponents / 8 - 19, r_maxglslbones->integer );
#else
	// require GLSL 1.20+ for GPU skinning
	if( glConfig.shadingLanguageVersion >= 120 ) {
		// the maximum amount of bones we can handle in a vertex shader (2 vec4 uniforms per vertex)
		glConfig.maxGLSLBones = bound( 0, glConfig.maxVertexUniformComponents / 8 - 19, r_maxglslbones->integer );
	}
	else {
		glConfig.maxGLSLBones = 0;
	}
#endif

#ifndef GL_ES_VERSION_2_0
	if( glConfig.ext.texture_non_power_of_two )
	{
		// blacklist this extension on Radeon X1600-X1950 hardware (they support it only with certain filtering/repeat modes)
		val = 0;

		// LadyHavoc: this is blocked on Mac OS X because the drivers claim support but often can't accelerate it or crash when used.
#ifndef __APPLE__
		if( glConfig.ext.vertex_shader )
			qglGetIntegerv( GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB, &val );
#endif

		if( val <= 0 )
			glConfig.ext.texture_non_power_of_two = false;
	}
#endif

	if( glConfig.ext.depth24 ) {
		glConfig.depthEpsilon = 1.0 / (1<<22);
	}
	else {
		glConfig.depthEpsilon = 1.0 / (1<<14);
	}

	cvar = ri.Cvar_Get( "gl_ext_vertex_buffer_object_hack", "0", CVAR_ARCHIVE|CVAR_NOSET );
	if( cvar && !cvar->integer ) 
	{
		ri.Cvar_ForceSet( cvar->name, "1" );
		ri.Cvar_ForceSet( "gl_ext_vertex_buffer_object", "1" );
	}

#ifdef GL_ES_VERSION_2_0
	// Use 32-bit framebuffers on PowerVR instead of 24-bit with the alpha cleared to 1
	// because blending incorrectly assumes alpha 0 when an RGB FB is used there, not 1.
	if( !strcmp( glConfig.vendorString, "Imagination Technologies" ) )
		glConfig.forceRGBAFramebuffers = true;
#endif

	ri.Cvar_Get( "r_texturefilter_max", "0", CVAR_READONLY );
	ri.Cvar_ForceSet( "r_texturefilter_max", va_r( tmp, sizeof( tmp ), "%i", glConfig.maxTextureFilterAnisotropic ) );

	ri.Cvar_Get( "r_soft_particles_available", "0", CVAR_READONLY );
	if( glConfig.ext.depth_texture && glConfig.ext.fragment_precision_high && glConfig.ext.framebuffer_blit )
		ri.Cvar_ForceSet( "r_soft_particles_available", "1" );

	// don't allow too high values for lightmap block size as they negatively impact performance
	if( r_lighting_maxlmblocksize->integer > glConfig.maxTextureSize / 4 &&
		!(glConfig.maxTextureSize / 4 < min(QF_LIGHTMAP_WIDTH,QF_LIGHTMAP_HEIGHT)*2) ) {
		ri.Cvar_ForceSet( "r_lighting_maxlmblocksize", va_r( tmp, sizeof( tmp ), "%i", glConfig.maxTextureSize / 4 ) );
  }

	return true;
}

/*
* R_PrintGLExtensionsString
*/
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

void GLimp_PrintConfig()
{
	int i;
	size_t lastOffset;
	const gl_extension_t *extension;

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

	Com_Printf( "mode: %ix%i%s\n", r_renderer_state.width, r_renderer_state.height, r_renderer_state.fullScreen ? ", fullscreen" : ", windowed" );
	Com_Printf( "picmip: %i\n", r_picmip->integer );
	Com_Printf( "texturemode: %s\n", r_texturemode->string );
	Com_Printf( "anisotropic filtering: %i\n", r_texturefilter->integer );
	Com_Printf( "vertical sync: %s\n", ( r_swapinterval->integer || r_swapinterval_min->integer ) ? "enabled" : "disabled" );
	Com_Printf( "multithreading: %s\n", glConfig.multithreading ? "enabled" : "disabled" );

	for( i = 0, lastOffset = 0, extension = gl_extensions_decl; i < num_gl_extensions; i++, extension++ ) {
		if( lastOffset != extension->offset ) {
			lastOffset = extension->offset;
			Com_Printf( "%s: %s\n", extension->name, GLINF_FROM( &glConfig.ext, lastOffset ) ? "enabled" : "disabled" );
		}
	}
}
