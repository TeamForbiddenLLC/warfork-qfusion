#include "include/global.glsl"

layout(set = DESCRIPTOR_OBJECT_SET, binding = 4) uniform DefaultCellShadeCB pass;

layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) out vec3 v_TexCoordCube;
layout(location = 2) out vec2 v_FogCoord;

void main(void)
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec2 TexCoord = a_TexCoord;
	myhalf4 inColor = myhalf4(a_Color);

	QF_TransformVerts(Position, Normal, TexCoord);

	myhalf4 outColor = QF_VertexRGBGen(Position, Normal, inColor);

#ifdef APPLY_FOG
	#ifdef APPLY_FOG_COLOR
		QF_FogGenColor(Position, outColor, obj.blendMix);
	#else
		QF_FogGenCoordTexCoord(Position, v_FogCoord);
	#endif
#endif

	qf_FrontColor = vec4(outColor);

#if defined(APPLY_TC_MOD)
	v_TexCoord = TextureMatrix2x3Mul(pass.textureMatrix, TexCoord);
#else
	v_TexCoord = TexCoord;
#endif
	v_TexCoordCube = pass.reflectionTexMatrix * reflect(normalize(Position.xyz - obj.entityDist), Normal.xyz);

	gl_Position = obj.mvp * Position;
}
