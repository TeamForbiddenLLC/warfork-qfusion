#include "include/resource.glsl"

layout(set = 2, binding = 0) uniform UBODefaultCellShade ubo;  
layout(set = 2, binding = 1) uniform FogUniforms fog;  

layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) out vec3 v_TexCoordCube;

#if defined(APPLY_FOG) && !defined(APPLY_FOG_COLOR)
	layout(location = 2) out vec2 v_FogCoord;
#endif

#include "include/common.glsl"
#include "include/attributes.glsl"
#include "include/rgbgen.glsl"

void main(void)
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec2 TexCoord = a_TexCoord;
	myhalf4 inColor = myhalf4(a_Color);

	QF_TransformVerts(Position, Normal, TexCoord);

	myhalf4 outColor = VertexRGBGen(Position, Normal, inColor);

#ifdef APPLY_FOG
	#ifdef APPLY_FOG_COLOR
		FogGenColor(fog, Position, outColor, ubo.blendMix);
	#else
		FogGenCoordTexCoord(fog, Position, v_FogCoord);
	#endif
#endif

	qf_FrontColor = vec4(outColor);

#if defined(APPLY_TC_MOD)
	v_TexCoord = TextureMatrix2x3Mul(ubo.textureMatrix, TexCoord);
#else
	v_TexCoord = TexCoord;
#endif
	v_TexCoordCube = ubo.reflectionTextureMat * reflect(normalize(Position.xyz - ubo.entityDist), Normal.xyz);

	gl_Position = ubo.mvp * Position;
}
