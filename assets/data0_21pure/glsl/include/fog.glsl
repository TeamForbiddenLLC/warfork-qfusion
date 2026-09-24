uniform vec2 u_FogScaleAndEyeDist;
uniform vec4 u_FogEyePlane, u_FogPlane;
uniform vec3 u_FogColor;

#define FOG_TEXCOORD_STEP 1.0/256.0

#define FogDensity(coord) sqrt(clamp((coord)[0],0.0,1.0))*step(FOG_TEXCOORD_STEP,(coord)[1])

#define FOG_GEN_OUTPUT_COLOR
#include "fog_overload.glsl"

#undef FOG_GEN_OUTPUT_COLOR
#define FOG_GEN_OUTPUT_TEXCOORDS
#include "fog_overload.glsl"

/* ============================================================
 * Atmospheric Fog Evaluation
 * Appended below legacy BSP fog plane system.
 * ============================================================ */

/*
 * EvaluateAtmosphericFog
 * Evaluates extinction, ground mist, sun inscattering, and sky horizon blending for OpenGL.
 */
vec4 EvaluateAtmosphericFog(in vec3 worldPos, in vec3 cameraPos)
{
	if (u_AtmFogColor.a <= 0.0001)
		return vec4(0.0);

	vec3 dir = worldPos - cameraPos;
	float dist = length(dir);
	if (dist <= 0.0001)
		return vec4(0.0);

	vec3 viewDir = dir / dist;

	/* Distance extinction (Beer-Lambert law) */
	float effDist = max(0.0, dist - u_AtmFogDistParams.x);
	float factor = 1.0 - exp(-effDist * u_AtmFogDistParams.w);

	/* Vertical height attenuation (clear -> full ramp, order encodes direction) */
	if (u_AtmFogHeightParams.w > 0.5)
	{
		float invRange = u_AtmFogHeightParams.z;
		float hWeight = clamp((worldPos.z - u_AtmFogHeightParams.x) * invRange, 0.0, 1.0);
		factor *= hWeight;
	}

	factor = clamp(factor, 0.0, 1.0);
	vec3 fogColor = u_AtmFogColor.rgb;

	/* Directional Sun Inscattering (Mie glow) */
	if (u_AtmFogSunParams.w > 0.0)
	{
		float sunDot = max(0.0, dot(viewDir, u_AtmFogSunParams.xyz));
		float mieGlow = pow(sunDot, u_AtmFogSunColor.w) * u_AtmFogSunParams.w;
		fogColor += u_AtmFogSunColor.rgb * mieGlow * factor;
	}

	/* Sky Horizon Blending */
	if (u_AtmFogSkyParams.w > 0.5)
	{
		float zenithAngle = max(0.0, viewDir.z + u_AtmFogSkyParams.x);
		float horizonFactor = clamp(pow(zenithAngle, u_AtmFogSkyParams.z) * u_AtmFogSkyParams.y, 0.0, 1.0);
		fogColor = mix(fogColor, u_AtmFogColor.rgb, horizonFactor);
	}

	return vec4(fogColor, factor * u_AtmFogColor.a);
}
