uniform vec4 u_DeluxemapOffset[(NUM_LIGHTMAPS + 3) / 4]; // s-offset for v_LightmapTexCoord
uniform LightmapSampler u_LightmapTexture0;
#if NUM_LIGHTMAPS >= 2
uniform LightmapSampler u_LightmapTexture1;
#if NUM_LIGHTMAPS >= 3
uniform LightmapSampler u_LightmapTexture2;
#if NUM_LIGHTMAPS >= 4
uniform LightmapSampler u_LightmapTexture3;
#endif // NUM_LIGHTMAPS >= 4
#endif // NUM_LIGHTMAPS >= 3
#endif // NUM_LIGHTMAPS >= 2

// deluxemapping using light vectors in modelspace
vec3 LightmapColor(in vec3 surfaceNormalModelspace, out vec3 weightedDiffuseNormalModelspace)
{
	vec3 diffuseNormalModelspace;
	float diffuseProduct;
	vec3 color = vec3(0.0);
	
	// get light normal
	diffuseNormalModelspace = normalize(vec3 (Lightmap(u_LightmapTexture0, v_LightmapTexCoord01.st+vec2(u_DeluxemapOffset[0].x, 0.0), v_LightmapLayer0123.x)) - vec3 (0.5));
	// calculate directional shading
	diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));

#ifdef APPLY_FBLIGHTMAP
	weightedDiffuseNormalModelspace = diffuseNormalModelspace;
	// apply lightmap color
	color.rgb += vec3 (max (diffuseProduct, 0.0) * vec3 (Lightmap(u_LightmapTexture0, v_LightmapTexCoord01.st, v_LightmapLayer0123.x)));
#else
#define NORMALIZE_DIFFUSE_NORMAL
	weightedDiffuseNormalModelspace = u_LightstyleColor[0] * diffuseNormalModelspace;
	// apply lightmap color
	color.rgb += u_LightstyleColor[0] * float(max (diffuseProduct, 0.0)) * vec3 (Lightmap(u_LightmapTexture0, v_LightmapTexCoord01.st, v_LightmapLayer0123.x));
#endif // APPLY_FBLIGHTMAP

#ifdef APPLY_AMBIENT_COMPENSATION
	// compensate for ambient lighting
	color.rgb += float((1.0 - max (diffuseProduct, 0.0))) * u_LightAmbient;
#endif

#if NUM_LIGHTMAPS >= 2
	diffuseNormalModelspace = normalize(vec3 (Lightmap(u_LightmapTexture1, v_LightmapTexCoord01.pq+vec2(u_DeluxemapOffset[0].y,0.0), v_LightmapLayer0123.y)) - vec3 (0.5));
	diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));
	weightedDiffuseNormalModelspace += u_LightstyleColor[1] * diffuseNormalModelspace;
	color.rgb += u_LightstyleColor[1] * float(max (diffuseProduct, 0.0)) * vec3 (Lightmap(u_LightmapTexture1, v_LightmapTexCoord01.pq, v_LightmapLayer0123.y));
#if NUM_LIGHTMAPS >= 3
	diffuseNormalModelspace = normalize(vec3 (Lightmap(u_LightmapTexture2, v_LightmapTexCoord23.st+vec2(u_DeluxemapOffset[0].z,0.0), v_LightmapLayer0123.z)) - vec3 (0.5));
	diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));
	weightedDiffuseNormalModelspace += u_LightstyleColor[2] * diffuseNormalModelspace;
	color.rgb += u_LightstyleColor[2] * float(max (diffuseProduct, 0.0)) * vec3 (Lightmap(u_LightmapTexture2, v_LightmapTexCoord23.st, v_LightmapLayer0123.z));
#if NUM_LIGHTMAPS >= 4
	diffuseNormalModelspace = normalize(vec3 (Lightmap(u_LightmapTexture3, v_LightmapTexCoord23.pq+vec2(u_DeluxemapOffset[0].w,0.0), v_LightmapLayer0123.w)) - vec3 (0.5));
	diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));
	weightedDiffuseNormalModelspace += u_LightstyleColor[3] * diffuseNormalModelspace;
	color.rgb += u_LightstyleColor[3] * float(max (diffuseProduct, 0.0)) * vec3 (Lightmap(u_LightmapTexture3, v_LightmapTexCoord23.pq, v_LightmapLayer0123.w));
#endif // NUM_LIGHTMAPS >= 4
#endif // NUM_LIGHTMAPS >= 3
#endif // NUM_LIGHTMAPS >= 2

	return color;
}
