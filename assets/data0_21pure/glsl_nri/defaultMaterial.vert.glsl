#include "include/global.glsl" 

layout(set = DESCRIPTOR_OBJECT_SET, binding = 4) uniform DefaultMaterialCB pass;

layout(location = 0) out vec3 v_Position 
layout(location = 1) out vec4 v_EyeVector 
layout(location = 2) out qf_lmvec01 v_LightmapTexCoord01;
layout(location = 3) out qf_lmvec23 v_LightmapTexCoord23;
layout(location = 5) flat out ivec4 v_LightmapLayer0123;
layout(location = 6) out mat3 v_StrMatrix; // directions of S/T/R texcoords (tangent, binormal, normal)
layout(location = 7) out vec4 frontColor; 

void main()
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec4 inColor = vec4(a_Color);
	vec2 TexCoord = a_TexCoord;
	vec3 Tangent = a_SVector.xyz;
	float TangentDir = a_SVector.w;

	QF_TransformVerts_Tangent(Position, Normal, Tangent, TexCoord);

	vec4 outColor = QF_VertexRGBGen(Position, Normal, inColor);

#ifdef APPLY_FOG
#if defined(APPLY_FOG_COLOR)
	QF_FogGenColor(Position, outColor);
#else
	QF_FogGenCoordTexCoord(Position, v_TexCoord_FogCoord.pq);
#endif
#endif // APPLY_FOG

	frontColor = vec4(outColor);

#if defined(APPLY_TC_MOD)
	v_TexCoord_FogCoord.st = TextureMatrix2x3Mul(pass.textureMatrix, TexCoord);
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

	gl_Position = obj.mvp * Position;
}
