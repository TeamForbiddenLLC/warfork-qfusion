layout(location = 10) qf_varying vec4 v_TexCoord_FogCoord;
#ifdef NUM_LIGHTMAPS
layout(location = 11) qf_varying qf_lmvec01 v_LightmapTexCoord01;
#if NUM_LIGHTMAPS > 2
layout(location = 12) qf_varying qf_lmvec23 v_LightmapTexCoord23;
#endif
#ifdef LIGHTMAP_ARRAYS
layout(location = 13)qf_flat_varying vec4 v_LightmapLayer0123;
#endif
#endif

#if defined(NUM_DLIGHTS) || defined(APPLY_SPECULAR)
layout(location = 14) qf_varying vec3 v_Position;
#endif

#if defined(APPLY_SPECULAR) || defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)
layout(location = 15) qf_varying vec3 v_EyeVector;
#endif

layout(location = 16) qf_varying mat3 v_StrMatrix; // directions of S/T/R texcoords (tangent, binormal, normal)
