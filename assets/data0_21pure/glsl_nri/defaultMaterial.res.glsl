layout(set = DESCRIPTOR_OBJECT_SET, binding = 4) uniform DefaultMaterialCB {
  vec4 entityColor;
  vec4 textureMatrix[2];
  vec4 lightstyleColor[4];
  vec4 deluxLightMapScale;
  float offsetScale;
  vec3 lightDir;
  float glossIntensity;
  vec3 floorColor;
  float glossExponent;
  vec3 wallColor;
} pass;

layout(set = DESCRIPTOR_OBJECT_SET, binding = 5) uniform DynamicLightCB {
  int numberLights;
  DynLight dynLights[16];
} lights; 

