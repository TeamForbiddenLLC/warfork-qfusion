#ifndef FOG_DIST_NUDGE_FACTOR
#define FOG_DIST_NUDGE_FACTOR 0.001
#endif

#if defined(FOG_GEN_OUTPUT_COLOR)
void FogGenColor(in vec4 Position, inout vec4 outColor, in vec2 blendMix)
#elif defined(FOG_GEN_OUTPUT_TEXCOORDS)
void FogGenCoord(in vec4 Position, out vec2 outTexCoord)
#endif
{
	// side = vec2(inside, outside)
	vec2 side = vec2(step(u_FogScaleAndEyeDist.y, 0.0), step(0.0, u_FogScaleAndEyeDist.y));
	float FDist = dot(Position.xyz, u_FogEyePlane.xyz) - u_FogEyePlane.w;
	float FVdist = dot(Position.xyz, u_FogPlane.xyz) - u_FogPlane.w;
	float VToEyeDist = FVdist - u_FogScaleAndEyeDist.y;

	// prevent calculations that might result in infinities:
	// always ensure that abs(NudgedVToEyeDist) >= FOG_DIST_NUDGE_FACTOR
	float NudgedVToEyeDist = step(FOG_DIST_NUDGE_FACTOR, VToEyeDist    ) * VToEyeDist +
				step(FOG_DIST_NUDGE_FACTOR, VToEyeDist * -1.0) * VToEyeDist + 
				(step(abs(VToEyeDist), FOG_DIST_NUDGE_FACTOR)) * FOG_DIST_NUDGE_FACTOR;

	float FogDistScale = FVdist / NudgedVToEyeDist;

#if defined(FOG_GEN_OUTPUT_COLOR)
	float FogDist = FDist * dot(side, vec2(1.0, FogDistScale));
	float FogScale = float(clamp(1.0 - FogDist * u_FogScaleAndEyeDist.x, 0.0, 1.0));
	outColor *= mix(vec4(1.0), vec4(FogScale), blendMix.xxxy);
#endif

#if defined(FOG_GEN_OUTPUT_TEXCOORDS)
	float FogS = FDist * dot(side, vec2(1.0, step(FVdist, 0.0) * FogDistScale));
	float FogT = -FVdist;
	outTexCoord = vec2(FogS * u_FogScaleAndEyeDist.x, FogT * u_FogScaleAndEyeDist.x + 1.5*FOG_TEXCOORD_STEP);
#endif
}
