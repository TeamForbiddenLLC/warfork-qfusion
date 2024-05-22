myhalf3 CelShading(in myhalf3 surfaceNormalModelspace, in myhalf3 diffuseNormalModelspace)
{ 
	myhalf diffuseProductPositive;
	myhalf diffuseProductNegative;
	myhalf diffuseProduct;
	myhalf hardShadow = 0.0;

#ifdef APPLY_HALFLAMBERT
	diffuseProduct = myhalf (dot (surfaceNormalModelspace, diffuseNormalModelspace));
	diffuseProductPositive = myhalf ( clamp(diffuseProduct, 0.0, 1.0) * 0.5 + 0.5 );
	diffuseProductPositive *= diffuseProductPositive;
	diffuseProductNegative = myhalf ( clamp(diffuseProduct, -1.0, 0.0) * 0.5 - 0.5 );
	diffuseProductNegative = diffuseProductNegative * diffuseProductNegative - 0.25;
	diffuseProduct = diffuseProductPositive;
#else
	diffuseProduct = myhalf (dot (surfaceNormalModelspace, diffuseNormalModelspace));
	diffuseProductPositive = max (diffuseProduct, 0.0);
	diffuseProductNegative = (-min (diffuseProduct, 0.0) - 0.3);
#endif // APPLY_HALFLAMBERT

	// smooth the hard shadow edge
	hardShadow += floor(max(diffuseProduct + 0.1, 0.0) * 2.0);
	hardShadow += floor(max(diffuseProduct + 0.055, 0.0) * 2.0);
	hardShadow += floor(diffuseProductPositive * 2.0);

	diffuseProduct = myhalf(0.6 + hardShadow * 0.09 + diffuseProductPositive * 0.14);

	// backlight
	diffuseProduct += myhalf (ceil(diffuseProductNegative * 2.0) * 0.085 + diffuseProductNegative * 0.085);

	return myhalf3(diffuseProduct);
}

myhalf3 DirectionalLightColor(in myhalf3 surfaceNormalModelspace, out myhalf3 weightedDiffuseNormalModelspace)
{
	myhalf3 diffuseNormalModelspace;
	myhalf diffuseProduct;
	myhalf3 color = myhalf3(0.0);

#ifdef APPLY_DIRECTIONAL_LIGHT_FROM_NORMAL
	diffuseNormalModelspace = v_StrMatrix[2];
#else
	diffuseNormalModelspace = u_LightDir;
#endif // APPLY_DIRECTIONAL_LIGHT_FROM_NORMAL

	weightedDiffuseNormalModelspace = diffuseNormalModelspace;

#ifdef APPLY_CELSHADING

	color.rgb += CelShading(surfaceNormalModelspace, diffuseNormalModelspace);
	
#else

#ifdef APPLY_HALFLAMBERT
	diffuseProduct = float ( clamp(dot (surfaceNormalModelspace, diffuseNormalModelspace), 0.0, 1.0) * 0.5 + 0.5 );
	diffuseProduct *= diffuseProduct;
#else
	diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));
#endif // APPLY_HALFLAMBERT

#ifdef APPLY_DIRECTIONAL_LIGHT_MIX
	color.rgb += qf_FrontColor.rgb;
#else
	color.rgb += u_LightDiffuse.rgb * myhalf(max (diffuseProduct, 0.0)) + u_LightAmbient;
#endif

#endif // APPLY_CELSHADING

	return color;
}
