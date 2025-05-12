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
layout(set = DESCRIPTOR_GLOBAL_SET, binding = 1) uniform texture2D lightmapTexture[16];

layout(location = 0) in vec3 v_Position; 
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;  
layout(location = 3) in vec4 frontColor; 

layout(location = 4) in vec4 v_LightmapTexCoord01;
layout(location = 5) in vec4 v_LightmapTexCoord23;
layout(location = 6) flat in uvec4 v_LightmapLayer0123;
layout(location = 7) in vec2 v_FogCoord;

layout(location = 0) out vec4 outFragColor;



// for non-uniform access
vec4 lightMapAccess(vec2 coord, uint index) {
	switch(index) {
		case 0:
			return texture(sampler2D(lightmapTexture[0],lightmapTextureSample), coord);
		case 1:
			return texture(sampler2D(lightmapTexture[1],lightmapTextureSample), coord);
		case 2:
			return texture(sampler2D(lightmapTexture[2],lightmapTextureSample), coord);
		case 3:
			return texture(sampler2D(lightmapTexture[3],lightmapTextureSample), coord);
		case 4:
			return texture(sampler2D(lightmapTexture[4],lightmapTextureSample), coord);
		case 5:
			return texture(sampler2D(lightmapTexture[5],lightmapTextureSample), coord);
		case 6:
			return texture(sampler2D(lightmapTexture[6],lightmapTextureSample), coord);
		case 7:
			return texture(sampler2D(lightmapTexture[7],lightmapTextureSample), coord);
		case 8:
			return texture(sampler2D(lightmapTexture[8],lightmapTextureSample), coord);
		case 9:
			return texture(sampler2D(lightmapTexture[9],lightmapTextureSample), coord);
		case 10:
			return texture(sampler2D(lightmapTexture[10],lightmapTextureSample), coord);
		case 11:
			return texture(sampler2D(lightmapTexture[11],lightmapTextureSample), coord);
		case 12:
			return texture(sampler2D(lightmapTexture[12],lightmapTextureSample), coord);
		case 13:
			return texture(sampler2D(lightmapTexture[13],lightmapTextureSample), coord);
		case 14:
			return texture(sampler2D(lightmapTexture[14],lightmapTextureSample), coord);
		case 15:
			return texture(sampler2D(lightmapTexture[15],lightmapTextureSample), coord);
	}
}


void main(void)
{
	vec4 color;

#ifdef NUM_LIGHTMAPS
	color = vec4(0.0, 0.0, 0.0, frontColor.a);
	color.rgb += lightMapAccess(v_LightmapTexCoord01.st, v_LightmapLayer0123.x).rgb * pass.lightstyleColor[0];
	#if NUM_LIGHTMAPS >= 2
		color.rgb += lightMapAccess(v_LightmapTexCoord01.pq, v_LightmapLayer0123.y).rgb * pass.lightstyleColor[1];
		#if NUM_LIGHTMAPS >= 3
			color.rgb += lightMapAccess(v_LightmapTexCoord23.st, v_LightmapLayer0123.z).rgb * pass.lightstyleColor[2];
			#if NUM_LIGHTMAPS >= 4
				color.rgb += lightMapAccess(v_LightmapTexCoord23.pq, v_LightmapLayer0123.w).rgb * pass.lightstyleColor[3];
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
{
	for (int dlight = 0; dlight < min(lights.numberLights, 16); dlight += 4)
	{
		vec3 STR0 = vec3(lights.dynLights[dlight].position.xyz - v_Position);
		vec3 STR1 = vec3(lights.dynLights[dlight + 1].position.xyz - v_Position);
		vec3 STR2 = vec3(lights.dynLights[dlight + 2].position.xyz - v_Position);
		vec3 STR3 = vec3(lights.dynLights[dlight + 3].position.xyz - v_Position);
		vec4 distance = vec4(length(STR0), length(STR1), length(STR2), length(STR3));
		vec4 falloff = clamp(vec4(1.0) - (distance / lights.dynLights[dlight + 3].diffuseAndInvRadius), 0.0, 1.0);

		falloff *= falloff;

		color.rgb += vec3(
			dot(lights.dynLights[dlight].diffuseAndInvRadius, falloff),
			dot(lights.dynLights[dlight + 1].diffuseAndInvRadius, falloff),
			dot(lights.dynLights[dlight + 2].diffuseAndInvRadius, falloff));
	}
}
#endif

	vec4 diffuse;

#if defined(APPLY_CUBEMAP)
	diffuse = texture(samplerCube(u_BaseTexture,u_BaseSampler), reflect(v_Position - obj.entityDist, normalize(v_Normal)));
#elif defined(APPLY_CUBEMAP_VERTEX)
	diffuse = texture(samplerCube(u_BaseTexture,u_BaseSampler), v_TexCoord);
#elif defined(APPLY_SURROUNDMAP)
	diffuse = texture(samplerCube(u_BaseTexture,u_BaseSampler), v_Position - obj.entityDist);
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
