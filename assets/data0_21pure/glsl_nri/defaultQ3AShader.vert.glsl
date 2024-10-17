#include "include/global.glsl"
#include "defaultQ3AShader.res.glsl"

#include "include/qf_vert_utils.glsl"

layout(location = 0) out vec3 v_Position; 
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_TexCoord;  
layout(location = 3) out vec4 frontColor; 

layout(location = 4) out vec4 v_LightmapTexCoord01;
layout(location = 5) out vec4 v_LightmapTexCoord23;
layout(location = 6) flat out ivec4 v_LightmapLayer0123;

void main(void)
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec2 TexCoord = a_TexCoord;
	vec4 inColor = vec4(a_Color);
	vec3 Tangent = vec3(0,0,0);

	QF_TransformVerts(Position, Normal, Tangent, TexCoord);

	vec4 outColor = QF_VertexRGBGen(Position, Normal, inColor);

#ifdef APPLY_FOG
	#if defined(APPLY_FOG_COLOR)
		QF_FogGenColor(Position, outColor);
	#else
		QF_FogGenCoordTexCoord(Position, v_FogCoord);
	#endif
#endif // APPLY_FOG

frontColor = vec4(outColor);

#if defined(APPLY_CUBEMAP_VERTEX)
	#if defined(APPLY_TC_GEN_CELSHADE)
		v_TexCoord = mat3(pass.genTexMatrix) * reflect(normalize(Position.xyz - obj.entityDist), Normal.xyz);
	#endif // defined(APPLY_TC_GEN_CELSHADE)
#elif !defined(APPLY_CUBEMAP) && !defined(APPLY_SURROUNDMAP)

	#if defined(APPLY_TC_GEN_ENV)
			vec3 Projection;

			Projection = u_EntityDist - Position.xyz;
			Projection = normalize(Projection);

			float Depth = dot(Normal.xyz, Projection) * 2.0;
			v_TexCoord = 0.5 + (Normal.yz * Depth - Projection.yz) * vec2(0.5, -0.5);
	#elif defined(APPLY_TC_GEN_VECTOR)
			v_TexCoord = vec2(Position * pass.genTexMatrix); // account for u_VectorTexMatrix being transposed
	#elif defined(APPLY_TC_GEN_PROJECTION)
			v_TexCoord = vec2(normalize(obj.mvp * Position) * 0.5 + vec4(0.5));
	#elif defined(APPLY_TC_MOD)
			v_TexCoord = TextureMatrix2x3Mul(obj.textureMatrix, TexCoord);
	#else
			v_TexCoord = TexCoord;
	#endif // defined(APPLY_TC_GEN)

#endif // !defined(APPLY_CUBEMAP) && !defined(APPLY_SURROUNDMAP)

#if defined(NUM_DLIGHTS) || defined(APPLY_CUBEMAP) || defined(APPLY_SURROUNDMAP)
	v_Position = Position.xyz;
#endif

#if defined(APPLY_CUBEMAP) || defined(APPLY_DRAWFLAT)
	v_Normal = Normal;
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

	gl_Position = obj.mvp * Position;

#if defined(APPLY_SOFT_PARTICLE)
	vec4 modelPos = obj.mv * Position;
	v_Depth = -modelPos.z;
#endif	
}
