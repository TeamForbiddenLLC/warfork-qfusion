#include "include/global.glsl"
#include "defaultShadowmap.res.glsl"

#ifndef NUM_SHADOWS
	#define NUM_SHADOWS 1
#endif

layout(location = 0) out vec3 v_Position[4]; 
layout(location = 5) out vec3 v_ShadowProjVector[4];
layout(location = 10) out vec3 v_Normal;

#include "include/qf_vert_utils.glsl"

void main(void)
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec2 TexCoord = a_TexCoord;

	QF_TransformVerts(Position, Normal, TexCoord);

	gl_Position = obj.mvp * Position;

	v_ShadowProjVector[0] = u_ShadowmapMatrix0 * Position;
	// a trick which allows us not to perform 'shadowmaptc = (shadowmaptc + 1.0) * 0.5' in the fragment shader
	v_ShadowProjVector[0].xyz = (v_ShadowProjVector[0].xyz + vec3(v_ShadowProjVector[0].w)) * 0.5;
	v_Position[0] = Position;
#if NUM_SHADOWS >= 2
	v_ShadowProjVector[1] = u_ShadowmapMatrix1 * Position;
	v_ShadowProjVector[1].xyz = (v_ShadowProjVector[1].xyz + vec3(v_ShadowProjVector[1].w)) * 0.5;
	v_Position[1] = Position;
	#if NUM_SHADOWS >= 3
		v_ShadowProjVector[2] = u_ShadowmapMatrix2 * Position;
		v_ShadowProjVector[2].xyz = (v_ShadowProjVector[2].xyz + vec3(v_ShadowProjVector[2].w)) * 0.5;
		v_Position[2] = Position;
		#if NUM_SHADOWS >= 4
			v_ShadowProjVector[3] = u_ShadowmapMatrix3 * Position;
			v_ShadowProjVector[3].xyz = (v_ShadowProjVector[3].xyz + vec3(v_ShadowProjVector[3].w)) * 0.5;
			v_Position[3] = Position;
		#endif // NUM_SHADOWS >= 4
	#endif // NUM_SHADOWS >= 3
#endif // NUM_SHADOWS >= 2

	# ifdef APPLY_SHADOW_NORMAL_CHECK
	v_Normal = Normal;
	# endif
}
