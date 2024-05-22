#include "include/common.glsl"
#include "include/resource.glsl"
#include "include/attributes.glsl"


layout(set = 2, binding = 0) uniform UBODefaultDistortion ubo;

layout(location = 0) out vec4 v_TexCoord;
layout(location = 1) out vec4 v_FrontColor;
layout(location = 2) out vec4 v_ProjVector;
layout(location = 3) out vec3 v_EyeVector;

void main(void)
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec2 TexCoord = a_TexCoord;
	vec3 Tangent = a_SVector.xyz;
	float TangentDir = a_SVector.w;
	myhalf4 inColor = myhalf4(a_Color);

	QF_TransformVerts(Position, Normal, TexCoord);

	v_FrontColor = vec4(VertexRGBGen(
		ubo.constColor
		ubo.rgbGenFuncArgs,
		ubo.lightAmbient,
		ubo.entityDist,
		ubo.lightDiffuse,
		ubo.lightDir, Position, Normal, inColor));

	v_TexCoord.st = TextureMatrix2x3Mul(ubo.textureMatrix, TexCoord);

	vec4 textureMatrix3_[2];
	textureMatrix3_[0] =  ubo.textureMatrix[0];
	textureMatrix3_[1] = -ubo.textureMatrix[1];
	v_TexCoord.pq = TextureMatrix2x3Mul(textureMatrix3_, TexCoord);

#ifdef APPLY_EYEDOT
	mat3 v_StrMatrix;
	v_StrMatrix[0] = Tangent;
	v_StrMatrix[2] = Normal;
	v_StrMatrix[1] = TangentDir * cross(Normal, Tangent);

	vec3 EyeVectorWorld = (ubo.viewOrigin - Position.xyz) * ubo.frontPlane;
	v_EyeVector = EyeVectorWorld * v_StrMatrix;
#endif

	gl_Position = ubo.mvp * Position;
	v_ProjVector = gl_Position;
}
