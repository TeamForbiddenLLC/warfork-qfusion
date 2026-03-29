#include "include/common.glsl"
#include "include/uniforms.glsl"
#include_if(APPLY_FOG) "include/fog.glsl"
#include_if(APPLY_GREYSCALE) "include/greyscale.glsl"

qf_varying vec2 v_TexCoord;
qf_varying vec3 v_TexCoordCube;

#if defined(APPLY_FOG) && !defined(APPLY_FOG_COLOR)
qf_varying vec2 v_FogCoord;
#endif

uniform sampler2D u_BaseTexture;
uniform samplerCube u_CelShadeTexture;

#ifdef APPLY_DIFFUSE
uniform sampler2D u_DiffuseTexture;
#endif
#ifdef APPLY_DECAL
uniform sampler2D u_DecalTexture;
#endif
#ifdef APPLY_ENTITY_DECAL
uniform sampler2D u_EntityDecalTexture;
#endif
#ifdef APPLY_STRIPES
uniform sampler2D u_StripesTexture;
#endif
#ifdef APPLY_CEL_LIGHT
uniform samplerCube u_CelLightTexture;
#endif

void main(void)
{
	vec4 inColor = vec4(qf_FrontColor);

	vec4 tempColor;

	vec4 outColor;
	outColor = vec4(qf_texture(u_BaseTexture, v_TexCoord));
#ifdef QF_ALPHATEST
	QF_ALPHATEST(outColor.a * inColor.a);
#endif

#ifdef APPLY_ENTITY_DECAL
#ifdef APPLY_ENTITY_DECAL_ADD
	outColor.rgb += vec3(u_EntityColor.rgb) * vec3(qf_texture(u_EntityDecalTexture, v_TexCoord));
#else
	tempColor = vec4(u_EntityColor.rgb, 1.0) * vec4(qf_texture(u_EntityDecalTexture, v_TexCoord));
	outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);
#endif
#endif // APPLY_ENTITY_DECAL

#ifdef APPLY_DIFFUSE
	outColor.rgb *= vec3(qf_texture(u_DiffuseTexture, v_TexCoord));
#endif

	outColor.rgb *= vec3(qf_textureCube(u_CelShadeTexture, v_TexCoordCube));

#ifdef APPLY_STRIPES
#ifdef APPLY_STRIPES_ADD
	outColor.rgb += vec3(u_EntityColor.rgb) * vec3(qf_texture(u_StripesTexture, v_TexCoord));
#else
	tempColor = vec4(u_EntityColor.rgb, 1.0) * vec4(qf_texture(u_StripesTexture, v_TexCoord));
	outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);
#endif
#endif // APPLY_STRIPES_ADD

#ifdef APPLY_CEL_LIGHT
#ifdef APPLY_CEL_LIGHT_ADD
	outColor.rgb += vec3(qf_textureCube(u_CelLightTexture, v_TexCoordCube));
#else
	tempColor = vec4(qf_textureCube(u_CelLightTexture, v_TexCoordCube));
	outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);
#endif
#endif // APPLY_CEL_LIGHT

#ifdef APPLY_DECAL
#ifdef APPLY_DECAL_ADD
	outColor.rgb += vec3(qf_texture(u_DecalTexture, v_TexCoord));
#else
	tempColor = vec4(qf_texture(u_DecalTexture, v_TexCoord));
	outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);
#endif
#endif // APPLY_DECAL

	outColor = vec4(inColor * outColor);

#ifdef APPLY_GREYSCALE
	outColor.rgb = Greyscale(outColor.rgb);
#endif

#if defined(APPLY_FOG) && !defined(APPLY_FOG_COLOR)
	float fogDensity = FogDensity(v_FogCoord);
	outColor.rgb = mix(outColor.rgb, u_FogColor, fogDensity);
#endif

	qf_FragColor = vec4(outColor);
}
