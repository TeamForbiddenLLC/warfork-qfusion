#ifdef VERTEX_SHADER
  layout(location = 0) in vec4 a_Position;
  layout(location = 1) in vec4 a_Normal;
  layout(location = 2) in vec4 a_SVector;
  layout(location = 3) in vec4 a_Color;
  layout(location = 4) in vec2 a_TexCoord;

  #ifdef QF_NUM_BONE_INFLUENCES
	layout(location = 6) in uvec4 a_BonesIndices;
    layout(location = 7) in vec4 a_BonesWeights;
  #else
    #ifdef NUM_LIGHTMAPS
		layout(location = 6) in vec4 a_LightmapCoord01;
		#if NUM_LIGHTMAPS > 2
			layout(location = 7) in vec4 a_LightmapCoord23;
		#endif
	#endif
  #endif
  
  	#ifdef LIGHTMAP_ARRAYS
  		layout(location = 8) in uvec4 a_LightmapLayer0123;		
	#else
		#define a_LightmapLayer0123 uvec4(0)
	#endif
	#if defined(APPLY_AUTOSPRITE) || defined(APPLY_AUTOSPRITE2)
		layout(location = 5) in vec4 a_SpritePoint;
	#else
		#define a_SpritePoint vec4(0.0)
	#endif

	#if defined(APPLY_AUTOSPRITE2)
		// layout(location = 5) in vec4 a_SpriteRightUpAxis;
		#define a_SpriteRightUpAxis a_SVector
	#else
		#define a_SpriteRightUpAxis vec4(0.0)
	#endif
	
#endif // VERTEX_SHADER

