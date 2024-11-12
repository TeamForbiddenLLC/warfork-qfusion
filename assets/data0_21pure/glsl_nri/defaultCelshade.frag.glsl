#include "include/global.glsl" 
#include "defaultCelshade.res.glsl"

#include "include/qf_vert_utils.glsl"

layout(set = DESCRIPTOR_PASS_SET, binding = 1) uniform texture2D u_BaseTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 2) uniform sampler u_BaseSampler;

layout(set = DESCRIPTOR_PASS_SET, binding = 3) uniform textureCube u_CelShadeTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 4) uniform sampler u_CelShadeSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 5) uniform texture2D u_DiffuseTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 6) uniform sampler u_DiffuseSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 7) uniform texture2D u_DecalTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 8) uniform sampler u_DecalSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 9) uniform texture2D u_EntityDecalTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 10) uniform sampler u_EntityDecalSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 11) uniform texture2D u_StripesTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 12) uniform sampler u_StripesSampler;
layout(set = DESCRIPTOR_PASS_SET, binding = 13) uniform textureCube u_CelLightTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 14) uniform sampler u_CelLightSampler;

layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in vec3 v_TexCoordCube;
layout(location = 2) in vec2 v_FogCoord;
layout(location = 3) in vec4 v_FrontColor; 

layout(location = 0) out vec4 outFragColor;

void main(void)
{
	vec4 inColor = vec4(v_FrontColor);
	vec4 tempColor;
	vec4 outColor = vec4(texture(sampler2D(u_BaseTexture, u_BaseSampler), v_TexCoord));
#ifdef QF_ALPHATEST
	QF_ALPHATEST(outColor.a * inColor.a);
#endif

#ifdef APPLY_ENTITY_DECAL
	#ifdef APPLY_ENTITY_DECAL_ADD
		outColor.rgb += vec3(pass.entityColor.rgb) * vec3(texture(sampler2D(u_EntityDecalTexture,u_EntityDecalSampler), v_TexCoord));
	#else
		tempColor = vec4(pass.entityColor.rgb, 1.0) * vec4(texture(sampler2D(u_EntityDecalTexture,u_EntityDecalSampler), v_TexCoord));
		outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);
	#endif
#endif // APPLY_ENTITY_DECAL

#ifdef APPLY_DIFFUSE
	outColor.rgb *= vec3(texture(u_DiffuseTexture, v_TexCoord));
#endif

	outColor.rgb *= vec3(texture(samplerCube(u_CelShadeTexture, u_CelShadeSampler), v_TexCoordCube));

#ifdef APPLY_STRIPES
#ifdef APPLY_STRIPES_ADD
	outColor.rgb += vec3(pass.entityColor.rgb) * vec3(texture(sampler2D(u_StripesTexture,u_StripesSampler), v_TexCoord));
#else
	tempColor = vec4(pass.entityColor.rgb, 1.0) * vec4(texture(sampler2D(u_StripesTexture,u_StripesSampler), v_TexCoord));
	outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);
#endif
#endif // APPLY_STRIPES_ADD

#ifdef APPLY_CEL_LIGHT
#ifdef APPLY_CEL_LIGHT_ADD
	outColor.rgb += vec3(texture(samplerCube(u_CelLightTexture,u_CelLightSampler), v_TexCoordCube));
#else
	tempColor = vec4(texture(samplerCube(u_CelLightTexture,u_CelLightSampler), v_TexCoordCube));
	outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);
#endif
#endif // APPLY_CEL_LIGHT

#ifdef APPLY_DECAL
#ifdef APPLY_DECAL_ADD
	outColor.rgb += vec3(texture(sampler2D(u_DecalTexture,u_DecalSampler), v_TexCoord));
#else
	tempColor = vec4(texture(sampler2D(u_DecalTexture,u_DecalSampler), v_TexCoord));
	outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);
#endif
#endif // APPLY_DECAL

	outColor = vec4(inColor * outColor);

#ifdef APPLY_GREYSCALE
	outColor.rgb = Greyscale(outColor.rgb);
#endif

#if defined(APPLY_FOG) && !defined(APPLY_FOG_COLOR)
	float fogDensity = FogDensity(v_FogCoord);
	outColor.rgb = mix(outColor.rgb, frame.fogColor, fogDensity);
#endif

	outFragColor = vec4(outColor);
}
