#include "include/common.glsl"
#include "include/lightmap.glsl"
#include "include/uniforms.glsl"
#include_if(NUM_DLIGHTS) "include/dlights.glsl"
#include_if(APPLY_FOG) "include/fog.glsl"
#include_if(APPLY_GREYSCALE) "include/greyscale.glsl"

#include "include/varying_q3a.glsl"

#if defined(APPLY_CUBEMAP) || defined(APPLY_CUBEMAP_VERTEX) || defined(APPLY_SURROUNDMAP)
uniform samplerCube u_BaseTexture;
#else
uniform sampler2D u_BaseTexture;
#endif

#ifdef APPLY_DRAWFLAT
uniform vec3 u_WallColor;
uniform vec3 u_FloorColor;
#endif

#ifdef NUM_LIGHTMAPS
uniform LightmapSampler u_LightmapTexture0;
#if NUM_LIGHTMAPS >= 2
uniform LightmapSampler u_LightmapTexture1;
#if NUM_LIGHTMAPS >= 3
uniform LightmapSampler u_LightmapTexture2;
#if NUM_LIGHTMAPS >= 4
uniform LightmapSampler u_LightmapTexture3;
#endif // NUM_LIGHTMAPS >= 4
#endif // NUM_LIGHTMAPS >= 3
#endif // NUM_LIGHTMAPS >= 2
#endif // NUM_LIGHTMAPS

#if defined(APPLY_SOFT_PARTICLE)
#include "include/softparticle.glsl"
uniform sampler2D u_DepthTexture;
#endif

void main(void)
{
	vec4 color;

#ifdef NUM_LIGHTMAPS
	color = vec4(0.0, 0.0, 0.0, qf_FrontColor.a);
	color.rgb += vec3(Lightmap(u_LightmapTexture0, v_LightmapTexCoord01.st, v_LightmapLayer0123.x)) * u_LightstyleColor[0];
#if NUM_LIGHTMAPS >= 2
	color.rgb += vec3(Lightmap(u_LightmapTexture1, v_LightmapTexCoord01.pq, v_LightmapLayer0123.y)) * u_LightstyleColor[1];
#if NUM_LIGHTMAPS >= 3
	color.rgb += vec3(Lightmap(u_LightmapTexture2, v_LightmapTexCoord23.st, v_LightmapLayer0123.z)) * u_LightstyleColor[2];
#if NUM_LIGHTMAPS >= 4
	color.rgb += vec3(Lightmap(u_LightmapTexture3, v_LightmapTexCoord23.pq, v_LightmapLayer0123.w)) * u_LightstyleColor[3];
#endif // NUM_LIGHTMAPS >= 4
#endif // NUM_LIGHTMAPS >= 3
#endif // NUM_LIGHTMAPS >= 2
#else
	color = vec4(qf_FrontColor);
#endif // NUM_LIGHTMAPS

#if defined(APPLY_FOG) && !defined(APPLY_FOG_COLOR)
	float fogDensity = FogDensity(v_FogCoord);
#endif

#if defined(NUM_DLIGHTS)
	color.rgb += DynamicLightsColor(v_Position);
#endif

	vec4 diffuse;

#if defined(APPLY_CUBEMAP)
	diffuse = vec4(qf_textureCube(u_BaseTexture, reflect(v_Position - u_EntityDist, normalize(v_Normal))));
#elif defined(APPLY_CUBEMAP_VERTEX)
	diffuse = vec4(qf_textureCube(u_BaseTexture, v_TexCoord));
#elif defined(APPLY_SURROUNDMAP)
	diffuse = vec4(qf_textureCube(u_BaseTexture, v_Position - u_EntityDist));
#else
	diffuse = vec4(qf_texture(u_BaseTexture, v_TexCoord));
#endif

#ifdef APPLY_DRAWFLAT
	float n = float(step(DRAWFLAT_NORMAL_STEP, abs(v_Normal.z)));
	diffuse.rgb = vec3(mix(u_WallColor, u_FloorColor, n));
#endif

#ifdef APPLY_ALPHA_MASK
	color.a *= diffuse.a;
#else
	color *= diffuse;
#endif

#ifdef NUM_LIGHTMAPS
	// so that team-colored shaders work
	color *= vec4(qf_FrontColor);
#endif

#ifdef APPLY_GREYSCALE
	color.rgb = Greyscale(color.rgb);
#endif

#if defined(APPLY_FOG) && !defined(APPLY_FOG_COLOR)
	color.rgb = mix(color.rgb, u_FogColor, fogDensity);
#endif

#if defined(APPLY_SOFT_PARTICLE)
	float softness = FragmentSoftness(v_Depth, u_DepthTexture, gl_FragCoord.xy, u_ZRange);
	color *= mix(vec4(1.0), vec4(softness), u_BlendMix.xxxy);
#endif

#ifdef QF_ALPHATEST
	QF_ALPHATEST(color.a);
#endif

	qf_FragColor = vec4(color);
}
