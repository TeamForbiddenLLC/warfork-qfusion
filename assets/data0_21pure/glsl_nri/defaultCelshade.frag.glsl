#include "include/global.glsl"
#include "include/resource.glsl"

layout(set = DESCRIPTOR_OBJECT_SET, binding = 4) uniform DefaultCellShadeCB pass;

layout(set = DESCRIPTOR_PASS_SET, binding = 1) uniform texture2D u_BaseTexture;
layout(set = DESCRIPTOR_PASS_SET, binding = 2) uniform textureCube u_CelShadeTexture;

#ifdef APPLY_DIFFUSE
	layout(set = DESCRIPTOR_PASS_SET, binding = 3) uniform texture2D u_DiffuseTexture;
#endif

#ifdef APPLY_DECAL
	layout(set = DESCRIPTOR_PASS_SET, binding = 4) uniform texture2D u_DecalTexture;
#endif

#ifdef APPLY_ENTITY_DECAL
	layout(set = DESCRIPTOR_PASS_SET, binding = 5) uniform texture2D u_EntityDecalTexture;
#endif

#ifdef APPLY_STRIPES
	layout(set = DESCRIPTOR_PASS_SET, binding = 6) uniform texture2D u_StripesTexture;
#endif

#ifdef APPLY_CEL_LIGHT
	layout(set = DESCRIPTOR_PASS_SET, binding = 7) uniform textureCube u_CelLightTexture;
#endif

layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in vec3 v_TexCoordCube;
#if defined(APPLY_FOG) && !defined(APPLY_FOG_COLOR)
	layout(location = 2) in vec2 v_FogCoord;
#endif

layout(location = 0) out vec4 outFragColor;

void main(void)
{
	vec4 inColor = myhalf4(qf_FrontColor);
	vec4 tempColor;
	vec4 outColor = myhalf4(qf_texture(u_BaseTexture, v_TexCoord));
#ifdef QF_ALPHATEST
	QF_ALPHATEST(outColor.a * inColor.a);
#endif

#ifdef APPLY_ENTITY_DECAL
	#ifdef APPLY_ENTITY_DECAL_ADD
		outColor.rgb += myhalf3(pass.entityColor.rgb) * myhalf3(qf_texture(u_EntityDecalTexture, v_TexCoord));
	#else
		tempColor = myhalf4(pass.entityColor.rgb, 1.0) * myhalf4(qf_texture(u_EntityDecalTexture, v_TexCoord));
		outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);
	#endif
#endif // APPLY_ENTITY_DECAL

#ifdef APPLY_DIFFUSE
	outColor.rgb *= myhalf3(qf_texture(u_DiffuseTexture, v_TexCoord));
#endif

	outColor.rgb *= myhalf3(qf_textureCube(u_CelShadeTexture, v_TexCoordCube));

#ifdef APPLY_STRIPES
#ifdef APPLY_STRIPES_ADD
	outColor.rgb += myhalf3(pass.entityColor.rgb) * myhalf3(qf_texture(u_StripesTexture, v_TexCoord));
#else
	tempColor = myhalf4(ubo.entityColor.rgb, 1.0) * myhalf4(qf_texture(u_StripesTexture, v_TexCoord));
	outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);
#endif
#endif // APPLY_STRIPES_ADD

#ifdef APPLY_CEL_LIGHT
#ifdef APPLY_CEL_LIGHT_ADD
	outColor.rgb += myhalf3(qf_textureCube(u_CelLightTexture, v_TexCoordCube));
#else
	tempColor = myhalf4(qf_textureCube(u_CelLightTexture, v_TexCoordCube));
	outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);
#endif
#endif // APPLY_CEL_LIGHT

#ifdef APPLY_DECAL
#ifdef APPLY_DECAL_ADD
	outColor.rgb += myhalf3(qf_texture(u_DecalTexture, v_TexCoord));
#else
	tempColor = myhalf4(qf_texture(u_DecalTexture, v_TexCoord));
	outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);
#endif
#endif // APPLY_DECAL

	outColor = myhalf4(inColor * outColor);

#ifdef APPLY_GREYSCALE
	outColor.rgb = Greyscale(outColor.rgb);
#endif

#if defined(APPLY_FOG) && !defined(APPLY_FOG_COLOR)
	myhalf fogDensity = FogDensity(v_FogCoord);
	outColor.rgb = mix(outColor.rgb, pass.fogColor, fogDensity);
#endif

	outFragColor = vec4(outColor);
}
