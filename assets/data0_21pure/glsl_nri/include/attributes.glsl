#ifdef VERTEX_SHADER

layout(location = 2) qf_attribute vec4 a_Position;
layout(location = 3) qf_attribute vec4 a_SVector;
layout(location = 4) qf_attribute vec4 a_Normal;
layout(location = 5) qf_attribute vec4 a_Color;
layout(location = 6) qf_attribute vec2 a_TexCoord;
# ifdef NUM_LIGHTMAPS
layout(location = 7) qf_attribute qf_lmvec01 a_LightmapCoord01;
# if NUM_LIGHTMAPS > 2
layout(location = 8) qf_attribute qf_lmvec23 a_LightmapCoord23;
# endif // NUM_LIGHTMAPS > 2
# ifdef LIGHTMAP_ARRAYS
layout(location = 9) qf_attribute vec4 a_LightmapLayer0123;
# endif // LIGHTMAP_ARRAYS
#endif // NUM_LIGHTMAPS

#endif // VERTEX_SHADER
