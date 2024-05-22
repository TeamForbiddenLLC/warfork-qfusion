#ifdef VERTEX_SHADER

  layout(location = 0) in vec4 a_Position;
  layout(location = 1) in vec4 a_SVector;
  layout(location = 2) in vec4 a_Normal;
  layout(location = 3) in vec4 a_Color;
  layout(location = 4) in vec2 a_TexCoord;
  # ifdef NUM_LIGHTMAPS
    layout(location = 5) in qf_lmvec01 a_LightmapCoord01;
    # if NUM_LIGHTMAPS > 2
    layout(location = 6) in qf_lmvec23 a_LightmapCoord23;
    # endif // NUM_LIGHTMAPS > 2
    # ifdef LIGHTMAP_ARRAYS
    layout(location = 7) in vec4 a_LightmapLayer0123;
    # endif // LIGHTMAP_ARRAYS
  #endif // NUM_LIGHTMAPS

#endif // VERTEX_SHADER
