#ifdef DLIGHTS_SURFACE_NORMAL_IN
vec3 DynamicLightsSurfaceColor(in vec3 Position, in vec3 surfaceNormalModelspace)
#else
vec3 DynamicLightsColor(in vec3 Position)
#endif
{
	vec3 Color = vec3(0.0);

#if NUM_DLIGHTS > 4 // prevent the compiler from possibly handling the NUM_DLIGHTS <= 4 case as a real loop
#if !defined(GL_ES) && (QF_GLSL_VERSION >= 330)
	for (int dlight = 0; dlight < u_NumDynamicLights; dlight += 4)
#else
	for (int dlight = 0; dlight < NUM_DLIGHTS; dlight += 4)
#endif
#else
#define dlight 0
#endif
	{
		vec3 STR0 = vec3(u_DlightPosition[dlight] - Position);
		vec3 STR1 = vec3(u_DlightPosition[dlight + 1] - Position);
		vec3 STR2 = vec3(u_DlightPosition[dlight + 2] - Position);
		vec3 STR3 = vec3(u_DlightPosition[dlight + 3] - Position);
		vec4 distance = vec4(length(STR0), length(STR1), length(STR2), length(STR3));
		vec4 falloff = clamp(vec4(1.0) - distance * u_DlightDiffuseAndInvRadius[dlight + 3], 0.0, 1.0);

		falloff *= falloff;

		#ifdef DLIGHTS_SURFACE_NORMAL_IN
		distance = vec4(1.0) / distance;
		falloff *= max(vec4(
			dot(STR0 * distance.xxx, surfaceNormalModelspace),
			dot(STR1 * distance.yyy, surfaceNormalModelspace),
			dot(STR2 * distance.zzz, surfaceNormalModelspace),
			dot(STR3 * distance.www, surfaceNormalModelspace)), 0.0);
		#endif

		Color += vec3(
			dot(u_DlightDiffuseAndInvRadius[dlight], falloff),
			dot(u_DlightDiffuseAndInvRadius[dlight + 1], falloff),
			dot(u_DlightDiffuseAndInvRadius[dlight + 2], falloff));
	}

	return Color;
#ifdef dlight
#undef dlight
#endif
}
