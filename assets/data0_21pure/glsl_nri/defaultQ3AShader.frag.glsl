#include "include/global.glsl"
#include "defaultQ3AShader.res.glsl"

#include "include/qf_vert_utils.glsl"

layout(set = DESCRIPTOR_PASS_SET, binding = 3) uniform sampler u_BaseSampler;
#if defined(APPLY_CUBEMAP) || defined(APPLY_CUBEMAP_VERTEX) || defined(APPLY_SURROUNDMAP)
	layout(set = DESCRIPTOR_PASS_SET, binding = 4) uniform textureCube u_BaseTexture;
#else
	layout(set = DESCRIPTOR_PASS_SET, binding = 5) uniform texture2D u_BaseTexture;
#endif
layout(set = DESCRIPTOR_PASS_SET, binding = 1) uniform sampler u_DepthSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 2) uniform texture2D u_DepthTexture;

layout(set = DESCRIPTOR_GLOBAL_SET, binding = 0) uniform sampler lightmapTextureSample;
layout(set = DESCRIPTOR_GLOBAL_SET, binding = 1) uniform texture2D lightmapTexture[4];

layout(location = 0) in vec3 v_Position; 
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;  
layout(location = 3) in vec4 frontColor; 

layout(location = 4) in vec4 v_LightmapTexCoord01;
layout(location = 5) in vec4 v_LightmapTexCoord23;
layout(location = 6) flat in ivec4 v_LightmapLayer0123;


layout(location = 0) out vec4 outFragColor;

void main(void)
{
	vec4 color;

#ifdef NUM_LIGHTMAPS
	color = vec4(0.0, 0.0, 0.0, frontColor.a);
	color.rgb += texture(sampler2D(lightmapTexture[0],lightmapTextureSample), v_LightmapTexCoord01.st).rgb * pass.lightstyleColor[0];
	#if NUM_LIGHTMAPS >= 2
		color.rgb += texture(sampler2D(lightmapTexture[1],lightmapTextureSample), v_LightmapTexCoord01.pq).rgb * pass.lightstyleColor[1];
		#if NUM_LIGHTMAPS >= 3
			color.rgb += texture(sampler2D(lightmapTexture[2],lightmapTextureSample), v_LightmapTexCoord23.st).rgb * pass.lightstyleColor[2];
			#if NUM_LIGHTMAPS >= 4
				color.rgb += texture(sampler2D(lightmapTexture[3],lightmapTextureSample), v_LightmapTexCoord23.pq).rgb * pass.lightstyleColor[3];
			#endif // NUM_LIGHTMAPS >= 4
		#endif // NUM_LIGHTMAPS >= 3
	#endif // NUM_LIGHTMAPS >= 2
#else
	color = vec4(frontColor);
#endif // NUM_LIGHTMAPS

#if defined(APPLY_FOG) && !defined(APPLY_FOG_COLOR)
	float fogDensity = FogDensity(v_FogCoord);
#endif

#if defined(NUM_DLIGHTS)
	color.rgb += DynamicLightsColor(v_Position);
#endif

	vec4 diffuse;

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
	float n = float(step(DRAWFLAT_NORMAL_STEP, abs(v_Normal.z)));
	diffuse.rgb = vec3(mix(pass.wallColor, pass.floorColor, n));
#endif

#ifdef APPLY_ALPHA_MASK
	color.a *= diffuse.a;
#else
	color *= diffuse;
#endif

#ifdef NUM_LIGHTMAPS
	// so that team-colored shaders work
	color *= vec4(frontColor);
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

		float fragdepth = ZRange.x*ZRange.y/(ZRange.y - texture(sampler2D(u_DepthTexture,u_DepthSampler), tc).r*(pass.zRange.y-pass.zRange.x));
		flaot partdepth = Depth;
		
		float d = max((fragdepth - partdepth) * pass.softParticlesScale, 0.0);
		float softness = 1.0 - min(1.0, d);
		softness *= softness;
		softness = 1.0 - softness * softness;
		if(obj.isAlphaBlending > 0) {
			color *= mix(vec4(1.0), vec4(softness), vec4(0,0,0,1));
		} else {
			color *= mix(vec4(1.0), vec4(softness), vec4(1,1,1,0));
		}

	}
#endif

#ifdef QF_ALPHATEST
	QF_ALPHATEST(color.a);
#endif

	outFragColor = vec4(color);
}
