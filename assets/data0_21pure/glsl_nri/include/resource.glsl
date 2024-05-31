
struct VertexColoringCB {
#ifdef APPLY_RGB_DISTANCERAMP
    vec4 rgbGenFuncArgs;
#endif
#ifdef APPLY_ALPHA_DISTANCERAMP
    vec4 alphaGenFuncArgs;
#endif  
#if defined(APPLY_RGB_CONST)
    vec4 colorConst;
#elif defined(APPLY_RGB_GEN_DIFFUSELIGHT)
    vec4 lightAmbient;
    vec4 lightDiffuse;
    vec3 lightDir;
#endif

};


struct FrameCB {
  vec4 fogEyePlane;
  vec4 fogPlane;

  vec3 viewOrigin;
  float shaderTime;
  vec2 blendMix;
  float fogScale;
  float eyeDist;
  float mirrorSide;
  vec3 fogColor;
  mat3 viewAxis;
};

struct ObjectCB {
	vec3 entityOrigin;
    float entityDist;
	mat4 mvp;
	float blendMix;
};

// pass
struct DefaultCellShadeCB {
    vec4 textureMatrix[2];
    vec3 entityColor;
	mat3 reflectionTexMatrix;
};

struct DefaultShadowCB {
    mat4 shadowmapMatrix0;
    mat4 shadowmapMatrix1;
    mat4 shadowmapMatrix2;
    mat4 shadowmapMatrix3;
    vec4 shadowDir[4];
    vec4 shadowParams[4];
    vec4 shadowAlpha[2];
    vec4 shadowEntitydist[4];
};


struct DefaultDistortionCB {
    vec4 textureParams;
    vec4 textureMatrix[2];
    float frontPlane;
};

struct DefaultQ3ShaderCB {
    vec3 wallColor;
    float softParticleScale;
    vec4 textureParam;
    vec4 textureMatrix[2];
    vec3 floorColor;
    vec2 zRange;
    mat4 genTexMatrix;
    vec3 lightstyleColor[4];
};

struct DefaultMaterialCB {
    vec4 entityColor;
    vec4 textureMatrix[2];
    vec3 lightstyleColor[4]
    vec4 deluxLightMapScale;
    float offsetScale;
    vec3 lightDir;
};

// fxaa 
struct DefaultFXAACB {
    mat4 mvp;
    vec4 textureMatrix[2];
};


