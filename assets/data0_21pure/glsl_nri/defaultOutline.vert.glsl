#include "include/global.glsl"

layout(location = 0) out vec2 v_FogCoord;
layout(location = 1) out vec4 frontColor; 

uniform float u_OutlineHeight;

void main(void)
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec2 TexCoord = a_TexCoord;
	vec4 inColor = vec4(a_Color);

	QF_TransformVerts(Position, Normal, TexCoord);

	Position += vec4(Normal * u_OutlineHeight, 0.0);
	gl_Position = u_ModelViewProjectionMatrix * Position;

	vec4 outColor = VertexRGBGen(Position, Normal, inColor);

#ifdef APPLY_FOG
	#if defined(APPLY_FOG_COLOR)
		QF_FogGenColor(Position, outColor,  obj.blendMix);
	#else
		QF_FogGenCoordTexCoord(Position, v_FogCoord);
	#endif
#endif // APPLY_FOG

	frontColor = vec4(outColor);
}
