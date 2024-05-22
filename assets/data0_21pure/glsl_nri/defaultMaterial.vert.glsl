#include "include/common.glsl"
#include "include/resource.glsl"

#include "include/attributes.glsl"
#include "include/rgbgen.glsl"
#include "include/fog.glsl"
#include "include/varying_material.glsl"

layout(set = 2, binding = 0) uniform DefaultMaterialConstBuffer cbObject;
layout(set = 2, binding = 1) uniform FogConstBuffer cbFog;

layout(location = 0) out vec3 v_Position 
layout(location = 1) out vec4 v_EyeVector 
#ifdef NUM_LIGHTMAPS
	layout(location = 2) out qf_lmvec01 v_LightmapTexCoord01;
	#if NUM_LIGHTMAPS > 2 
		layout(location = 3) out qf_lmvec23 v_LightmapTexCoord23;
	#endif 
  # ifdef LIGHTMAP_ARRAYS
  	layout(location = 4) out vec4 a_LightmapLayer0123;
  # endif // LIGHTMAP_ARRAYS
#endif
layout(location = 5) out vec4 v_LightmapLayer0123;
layout(location = 6) out mat3 v_StrMatrix; // directions of S/T/R texcoords (tangent, binormal, normal)

void main()
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	myhalf4 inColor = myhalf4(a_Color);
	vec2 TexCoord = a_TexCoord;
	vec3 Tangent = a_SVector.xyz;
	float TangentDir = a_SVector.w;

	QF_TransformVerts_Tangent(Position, Normal, Tangent, TexCoord);

	vec4 outColor = VertexRGBGen(
		cbObject.constColor
		cbObject.rgbGenFuncArgs,
		cbObject.lightAmbient,
		cbObject.entityDist,
		cbObject.lightDiffuse,
		cbObject.lightDir,
		Position, Normal, inColor);

#ifdef APPLY_FOG
#if defined(APPLY_FOG_COLOR)
	FogGenColor(
			cbFog.eyePlane,
			cbFog.plane,
			cbFog.scale,
			cbFog.eyeDist,
		Position, outColor, constants.blendMix);
#else
	FogGenCoord(
			cbFog.eyePlane,
			cbFog.plane,
			cbFog.scale,
			cbFog.eyeDist,
		Position, v_TexCoord_FogCoord.pq);
#endif
#endif // APPLY_FOG

	qf_FrontColor = vec4(outColor);

#if defined(APPLY_TC_MOD)
	v_TexCoord_FogCoord.st = TextureMatrix2x3Mul(cbObject.textureMatrix, TexCoord);
#else
	v_TexCoord_FogCoord.st = TexCoord;
#endif

#ifdef NUM_LIGHTMAPS
	v_LightmapTexCoord01 = a_LightmapCoord01;
	#if NUM_LIGHTMAPS > 2
		v_LightmapTexCoord23 = a_LightmapCoord23;
	#endif // NUM_LIGHTMAPS > 2
	#ifdef LIGHTMAP_ARRAYS
		v_LightmapLayer0123 = a_LightmapLayer0123;
	#endif // LIGHTMAP_ARRAYS
#endif // NUM_LIGHTMAPS

	v_StrMatrix[0] = Tangent;
	v_StrMatrix[2] = Normal;
	v_StrMatrix[1] = TangentDir * cross(Normal, Tangent);

#if defined(APPLY_SPECULAR) || defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)
	vec3 EyeVectorWorld = set_view.viewOrigin - Position.xyz;
	v_EyeVector = EyeVectorWorld * v_StrMatrix;
#endif

#if defined(NUM_DLIGHTS) || defined(APPLY_SPECULAR)
	v_Position = Position.xyz;
#endif

	gl_Position = ubo.mvp * Position;
}
