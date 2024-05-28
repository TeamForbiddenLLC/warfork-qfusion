#include "include/global.glsl"


layout(set = DESCRIPTOR_OBJECT_SET, binding = 4) uniform DefaultQ3ShaderCB pass;
layout(set = DESCRIPTOR_PASS_SET, binding = 3) uniform sampler u_BaseSampler;
#if defined(APPLY_CUBEMAP) || defined(APPLY_CUBEMAP_VERTEX) || defined(APPLY_SURROUNDMAP)
	layout(set = DESCRIPTOR_PASS_SET, binding = 4) uniform textureCube u_BaseTexture;
#else
	layout(set = DESCRIPTOR_PASS_SET, binding = 5) uniform texture2D u_BaseTexture;
#endif

layout(set = DESCRIPTOR_GLOBAL_SET, binding = 0) uniform sampler lightmapTextureSample;
layout(set = DESCRIPTOR_GLOBAL_SET, binding = 1 + (16 * 0)) uniform texture2D lightmapTexture0[16];
layout(set = DESCRIPTOR_GLOBAL_SET, binding = 1 + (16 * 1)) uniform texture2D lightmapTexture1[16];
layout(set = DESCRIPTOR_GLOBAL_SET, binding = 1 + (16 * 2)) uniform texture2D lightmapTexture2[16];
layout(set = DESCRIPTOR_GLOBAL_SET, binding = 1 + (16 * 3)) uniform texture2D lightmapTexture3[16];

layout(set = DESCRIPTOR_PASS_SET, binding = 1) uniform texture2D u_DepthTexture;

layout(location = 0) out vec4 outFragColor;

void main(void)
{
	vec4 color;

#ifdef NUM_LIGHTMAPS
	color = vec4(0.0, 0.0, 0.0, qf_FrontColor.a);
	color.rgb += vec3(Lightmap(lightmapTexture0, v_LightmapTexCoord01.st, v_LightmapLayer0123.x)) * u_LightstyleColor[0];
	#if NUM_LIGHTMAPS >= 2
		color.rgb += vec3(Lightmap(lightmapTexture1, v_LightmapTexCoord01.pq, v_LightmapLayer0123.y)) * u_LightstyleColor[1];
		#if NUM_LIGHTMAPS >= 3
			color.rgb += vec3(Lightmap(lightmapTexture2, v_LightmapTexCoord23.st, v_LightmapLayer0123.z)) * u_LightstyleColor[2];
			#if NUM_LIGHTMAPS >= 4
				color.rgb += vec3(Lightmap(lightmapTexture3, v_LightmapTexCoord23.pq, v_LightmapLayer0123.w)) * u_LightstyleColor[3];
			#endif // NUM_LIGHTMAPS >= 4
		#endif // NUM_LIGHTMAPS >= 3
	#endif // NUM_LIGHTMAPS >= 2
#else
	color = myhalf4(qf_FrontColor);
#endif // NUM_LIGHTMAPS

#if defined(APPLY_FOG) && !defined(APPLY_FOG_COLOR)
	float fogDensity = FogDensity(v_FogCoord);
#endif

#if defined(NUM_DLIGHTS)
	color.rgb += DynamicLightsColor(v_Position);
#endif

	myhalf4 diffuse;

#if defined(APPLY_CUBEMAP)
	diffuse = texture(samplerCube(u_BaseTexture,u_BaseSampler), reflect(v_Position - u_EntityDist, normalize(v_Normal)));
#elif defined(APPLY_CUBEMAP_VERTEX)
	diffuse = texture(samplerCube(u_BaseTexture,u_BaseSampler), v_TexCoord);
#elif defined(APPLY_SURROUNDMAP)
	diffuse = texture(samplerCube(u_BaseTexture,u_BaseSampler), v_Position - u_EntityDist);
#else
	diffuse = texture(sampler2D(u_BaseTexture,u_BaseSampler), v_TexCoord);
#endif

#ifdef APPLY_DRAWFLAT
	myhalf n = myhalf(step(DRAWFLAT_NORMAL_STEP, abs(v_Normal.z)));
	diffuse.rgb = myhalf3(mix(pass.wallColor, u_FloorColor, n));
#endif

#ifdef APPLY_ALPHA_MASK
	color.a *= diffuse.a;
#else
	color *= diffuse;
#endif

#ifdef NUM_LIGHTMAPS
	// so that team-colored shaders work
	color *= myhalf4(qf_FrontColor);
#endif

#ifdef APPLY_GREYSCALE
	color.rgb = Greyscale(color.rgb);
#endif

#if defined(APPLY_FOG) && !defined(APPLY_FOG_COLOR)
	color.rgb = mix(color.rgb, frame.fogColor, fogDensity);
#endif

#if defined(APPLY_SOFT_PARTICLE)
	{
		vec2 tc = ScreenCoord * pass.textureParam.zw;

		float fragdepth = ZRange.x*ZRange.y/(ZRange.y - qf_texture(DepthTexture, tc).r*(pass.zRange.y-pass.zRange.x));
		flaot partdepth = Depth;
		
		myhalf d = max((fragdepth - partdepth) * u_SoftParticlesScale, 0.0);
		myhalf softness = 1.0 - min(1.0, d);
		softness *= softness;
		softness = 1.0 - softness * softness;
		color *= mix(myhalf4(1.0), myhalf4(softness), obj.blendMix.xxxy);
	}
#endif

#ifdef QF_ALPHATEST
	QF_ALPHATEST(color.a);
#endif

	outFragColor = vec4(color);
}
