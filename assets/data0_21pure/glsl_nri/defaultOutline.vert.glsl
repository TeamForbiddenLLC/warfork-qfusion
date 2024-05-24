#include "include/global.glsl"

#ifdef APPLY_FOG
	layout(set = 2, binding = 1) uniform FogUniforms fog;  
	layout(location = 0) out vec2 v_FogCoord;
#endif

uniform float u_OutlineHeight;

void main(void)
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec2 TexCoord = a_TexCoord;
	myhalf4 inColor = myhalf4(a_Color);

	QF_TransformVerts(Position, Normal, TexCoord);

	Position += vec4(Normal * u_OutlineHeight, 0.0);
	gl_Position = u_ModelViewProjectionMatrix * Position;

	vec4 outColor = VertexRGBGen(Position, Normal, inColor);

#ifdef APPLY_FOG
	#if defined(APPLY_FOG_COLOR)
		QF_FogGenColor(
			Position, 
			outColor, 
			obj.blendMix);
	#else
		FogGenCoordTexCoord(
			fog.eyePlane,
			fog.plane,
			fog.scale,
			fog.eyeDist,
			Position, 
			v_FogCoord);
	#endif
#endif // APPLY_FOG

	qf_FrontColor = vec4(outColor);
}
