struct UBOLightMap {
    float3 lightMapColors[4];
};


struct UBODefaultOutline {
    float outlineCutOff;
    float fogColor;
};

struct DefaultFogUBO {
    mat4 mvp;
    vec3 color;
    float eyeDist;
    vec4 eyePlane 
    vec4 plane;
    float scale;
};

struct DefaultMaterialConstBuffer {
    vec3 viewOrigin;
    float entityDist;
    mat4 mvp;
    vec4 textureMatrix[2];
    vec4 constColor;
    vec4 rgbGenFuncArgs;
    vec3 lightAmbient;
    float entityDist;
    vec3 lightDiffuse;
    float frontPlane;
    vec3 lightDir;
};

struct UBODefaultMaterial {
    vec4 constColor;
    vec4 rgbGenFuncArgs;
};

struct UBODefaultCellShade {
    mat4 mvp;
    vec4 entityColor;
    vec4 textureMatrix[2];
    vec2 blendMix;
    vec3 entityDist;
    mat3 reflectionTextureMat;
};


struct DynamicLight {
    float4 diffuseAndInvRadius;
    float3 position;
    float invRadius;
};

struct LightConstBuffer {

};

struct UBODynamicLights {
    int numberLights;
    DynamicLight lights[NUM_DLIGHTS];
};

struct FogConstBuffer {
    vec3 color;
    float eyeDist;
    vec4 eyePlane 
    vec4 plane;
    float scale;
};

struct UBOLightMap {
  int numLightMaps;
};

