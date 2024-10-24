#include "include/global.glsl" 
#include "defaultOutline.res.glsl"

#include "include/qf_vert_utils.glsl"

layout(location = 0) out vec2 v_FogCoord;
layout(location = 1) out vec4 frontColor; 

void main()
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec2 TexCoord = a_TexCoord;
	vec4 inColor = vec4(a_Color);
	vec3 Tangent = vec3(0,0,0);

	QF_TransformVerts(Position, Normal, Tangent, TexCoord);

	Position += vec4(Normal * push.outlineHeight, 0.0);
	gl_Position = obj.mvp * Position;

	vec4 outColor = QF_VertexRGBGen(Position, Normal, inColor);

#ifdef APPLY_FOG
	#if defined(APPLY_FOG_COLOR)
		QF_FogGenColor(Position, outColor,  obj.blendMix);
	#else
		QF_FogGenCoordTexCoord(Position, v_FogCoord);
	#endif
#endif // APPLY_FOG

	frontColor = vec4(outColor);
}
