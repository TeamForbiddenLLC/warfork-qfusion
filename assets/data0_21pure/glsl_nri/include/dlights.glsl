
myhalf3 DynamicLightsSurfaceColorSurfaceNormal(UBODynamicLight unif, in vec3 Position, in myhalf3 surfaceNormalModelspace, float3 surfaceNormalModelspace) {
	myhalf3 Color = myhalf3(0.0);

	for (int dlight = 0; dlight < lights.numberLights; dlight += 4)
	{
		myhalf3 STR0 = myhalf3(unif.lights[dlight].position - Position);
		myhalf3 STR1 = myhalf3(unif.lights[dlight + 1].position - Position);
		myhalf3 STR2 = myhalf3(unif.lights[dlight + 2].position - Position);
		myhalf3 STR3 = myhalf3(unif.lights[dlight + 3].position - Position);
		myhalf4 distance = myhalf4(length(STR0), length(STR1), length(STR2), length(STR3));
		myhalf4 falloff = clamp(myhalf4(1.0) - distance * unif.lights[dlight + 3].diffuseAndInvRadius, 0.0, 1.0);

		falloff *= falloff;

		distance = myhalf4(1.0) / distance;
		falloff *= max(myhalf4(
			dot(STR0 * distance.xxx, surfaceNormalModelspace),
			dot(STR1 * distance.yyy, surfaceNormalModelspace),
			dot(STR2 * distance.zzz, surfaceNormalModelspace),
			dot(STR3 * distance.www, surfaceNormalModelspace)), 0.0);

		Color += myhalf3(
			dot(unif.lights[dlight].diffuseAndInvRadius, falloff),
			dot(unif.lights[dlight + 1].diffuseAndInvRadius[dlight + 1], falloff),
			dot(unif.lights[dlight + 2].diffuseAndInvRadius[dlight + 2], falloff));
	}

	return Color;

}

myhalf3 DynamicLightsSurfaceColor(UBODynamicLight unif, in vec3 Position, in myhalf3 surfaceNormalModelspace)
{
	myhalf3 Color = myhalf3(0.0);

	for (int dlight = 0; dlight < lights.numberLights; dlight += 4)
	{
		myhalf3 STR0 = myhalf3(unif.lights[dlight].position - Position);
		myhalf3 STR1 = myhalf3(unif.lights[dlight + 1].position - Position);
		myhalf3 STR2 = myhalf3(unif.lights[dlight + 2].position - Position);
		myhalf3 STR3 = myhalf3(unif.lights[dlight + 3].position - Position);
		myhalf4 distance = myhalf4(length(STR0), length(STR1), length(STR2), length(STR3));
		myhalf4 falloff = clamp(myhalf4(1.0) - distance * unif.lights[dlight + 3].diffuseAndInvRadius, 0.0, 1.0);

		falloff *= falloff;

		Color += myhalf3(
			dot(unif.lights[dlight].diffuseAndInvRadius, falloff),
			dot(unif.lights[dlight + 1].diffuseAndInvRadius[dlight + 1], falloff),
			dot(unif.lights[dlight + 2].diffuseAndInvRadius[dlight + 2], falloff));
	}

	return Color;
}
