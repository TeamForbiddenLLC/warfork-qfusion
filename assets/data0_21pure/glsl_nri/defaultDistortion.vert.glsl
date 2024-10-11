#include "include/global.glsl"
#include "defaultDistortion.res.glsl"

layout(location = 0) out vec4 v_TexCoord;
layout(location = 1) out vec4 v_FrontColor;
layout(location = 2) out vec4 v_ProjVector;
layout(location = 3) out vec3 v_EyeVector;

#include "include/qf_vert_utils.glsl"

void main(void)
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec2 TexCoord = a_TexCoord;
	vec3 Tangent = a_SVector.xyz;
	float TangentDir = a_SVector.w;
	vec4 inColor = vec4(a_Color);

	QF_TransformVerts(Position, Normal, Tangent, TexCoord);

	v_FrontColor = vec4(QF_VertexRGBGen( Position, Normal, inColor));

	v_TexCoord.st = TextureMatrix2x3Mul(obj.textureMatrix, TexCoord);

	vec4 textureMatrix3_[2];
	textureMatrix3_[0] =  obj.textureMatrix[0];
	textureMatrix3_[1] = -obj.textureMatrix[1];
	v_TexCoord.pq = TextureMatrix2x3Mul(textureMatrix3_, TexCoord);

#ifdef APPLY_EYEDOT
	mat3 v_StrMatrix;
	v_StrMatrix[0] = Tangent;
	v_StrMatrix[2] = Normal;
	v_StrMatrix[1] = TangentDir * cross(Normal, Tangent);

	vec3 EyeVectorWorld = (frame.viewOrigin - Position.xyz) * pass.frontPlane;
	v_EyeVector = EyeVectorWorld * v_StrMatrix;
#endif

	gl_Position = obj.mvp * Position;
	v_ProjVector = gl_Position;
}
