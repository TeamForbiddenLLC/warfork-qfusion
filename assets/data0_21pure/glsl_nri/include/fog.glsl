#ifndef FOG_GLSL_INCLUDED
#define FOG_GLSL_INCLUDED

/*
 * EvaluateAtmosphericFog
 * Evaluates extinction, ground mist, sun inscattering, and sky horizon blending for NRI/Vulkan.
 */
vec4 EvaluateAtmosphericFog(vec3 worldPos, vec3 cameraPos)
{
	if (frame.atmFogColor.a <= 0.0001)
		return vec4(0.0);

	vec3 dir = worldPos - cameraPos;
	float dist = length(dir);
	if (dist <= 0.0001)
		return vec4(0.0);

	vec3 viewDir = dir / dist;

	/* Distance extinction (Beer-Lambert law) */
	float effDist = max(0.0, dist - frame.atmFogDistParams.x);
	float factor = 1.0 - exp(-effDist * frame.atmFogDistParams.w);

	/* Vertical height attenuation (clear -> full ramp, order encodes direction) */
	if (frame.atmFogHeightParams.w > 0.5)
	{
		float invRange = frame.atmFogHeightParams.z;
		float hWeight = clamp((worldPos.z - frame.atmFogHeightParams.x) * invRange, 0.0, 1.0);
		factor *= hWeight;
	}

	factor = clamp(factor, 0.0, 1.0);
	vec3 fogColor = frame.atmFogColor.rgb;

	/* Directional Sun Inscattering (Mie glow) */
	if (frame.atmFogSunParams.w > 0.0)
	{
		float sunDot = max(0.0, dot(viewDir, frame.atmFogSunParams.xyz));
		float mieGlow = pow(sunDot, frame.atmFogSunColor.w) * frame.atmFogSunParams.w;
		fogColor += frame.atmFogSunColor.rgb * mieGlow * factor;
	}

	/* Sky Horizon Blending */
	if (frame.atmFogSkyParams.w > 0.5)
	{
		float zenithAngle = max(0.0, viewDir.z + frame.atmFogSkyParams.x);
		float horizonFactor = clamp(pow(zenithAngle, frame.atmFogSkyParams.z) * frame.atmFogSkyParams.y, 0.0, 1.0);
		fogColor = mix(fogColor, frame.atmFogColor.rgb, horizonFactor);
	}

	return vec4(fogColor, factor * frame.atmFogColor.a);
}

#endif // FOG_GLSL_INCLUDED
