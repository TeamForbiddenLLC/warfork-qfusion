#include "include/global.glsl" 
#include "defaultOutline.res.glsl"
#include "include/qf_vert_utils.glsl"

layout(location = 0) in vec4 v_FogCoord;
layout(location = 1) in vec4 frontColor;

layout(location = 0) out vec4 outFragColor;

void main(void)
{
#ifdef APPLY_OUTLINES_CUTOFF
	if (u_OutlineCutOff > 0.0 && (gl_FragCoord.z / gl_FragCoord.w > push.outlineCutoff))
		discard;
#endif

#if defined(APPLY_FOG) && !defined(APPLY_FOG_COLOR)
	float fogDensity = FogDensity(v_FogCoord);
	outFragColor = vec4(vec3(mix(frontColor.rgb, frame.fogColor, fogDensity)), 1.0);
#else
	outFragColor = vec4(frontColor);
#endif
}
