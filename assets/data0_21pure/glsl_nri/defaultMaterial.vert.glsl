#include "include/global.glsl" 
#include "defaultMaterial.res.glsl"

layout(location = 0) out vec3 v_Position; 
layout(location = 1) out vec3 v_EyeVector; 
layout(location = 2) out vec4 v_LightmapTexCoord01;
layout(location = 3) out vec4 v_LightmapTexCoord23;
layout(location = 4) flat out ivec4 v_LightmapLayer0123;
layout(location = 5) out vec4 frontColor; 
layout(location = 6) out vec4 v_TexCoord_FogCoord; 
layout(location = 7) out vec3 v_Tangent; 
layout(location = 8) out vec3 v_Normal; 
layout(location = 9) out vec3 v_Binormal; 

#include "include/qf_vert_utils.glsl"

void main()
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec4 inColor = vec4(a_Color);
	vec2 TexCoord = a_TexCoord;
	vec3 Tangent = a_SVector.xyz;
	float TangentDir = a_SVector.w;

	QF_TransformVerts(Position, Normal, Tangent, TexCoord);

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
	v_TexCoord_FogCoord.st = TextureMatrix2x3Mul(obj.textureMatrix, TexCoord);
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

	v_Tangent = Tangent;
	v_Normal = Normal;
	v_Binormal = TangentDir * cross(Normal, Tangent);

#if defined(APPLY_SPECULAR) || defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)
	mat3 strMat;
	strMat[0] = v_Tangent;
	strMat[2] = v_Normal;
	strMat[1] = v_Binormal;

	vec3 EyeVectorWorld = frame.viewOrigin - Position.xyz;
	v_EyeVector = EyeVectorWorld * strMat;
#endif

#if defined(NUM_DLIGHTS) || defined(APPLY_SPECULAR)
	v_Position = Position.xyz;
#endif

	gl_Position = obj.mvp * Position;
}
