#include "include/global.glsl" 
#include "defaultCelshade.res.glsl"

layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) out vec3 v_TexCoordCube;
layout(location = 2) out vec2 v_FogCoord;
layout(location = 3) out vec4 v_FrontColor; 

#include "include/qf_vert_utils.glsl"

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
	#ifdef APPLY_FOG_COLOR
		QF_FogGenColor(Position, outColor);
	#else
		QF_FogGenCoordTexCoord(Position, v_FogCoord);
	#endif
#endif

	v_FrontColor = vec4(outColor);

#if defined(APPLY_TC_MOD)
	v_TexCoord = TextureMatrix2x3Mul(obj.textureMatrix, TexCoord);
#else
	v_TexCoord = TexCoord;
#endif
	v_TexCoordCube = pass.reflectionTexMatrix * reflect(normalize(Position.xyz - obj.entityDist), Normal.xyz);

	gl_Position = obj.mvp * Position;
}
