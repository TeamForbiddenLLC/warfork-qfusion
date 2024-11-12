layout(set = DESCRIPTOR_OBJECT_SET, binding = 4) uniform DefaultMaterialCB {
  vec4 entityColor;
  vec3 lightstyleColor[4];
  vec4 deluxLightMapScale;
  vec3 lightDir;
  float glossIntensity;
  vec3 floorColor;
  float glossExponent;
  vec3 wallColor;
  float offsetScale;
} pass;

// layout(set = DESCRIPTOR_OBJECT_SET, binding = 5) uniform DynamicLightCB {
//  int numberLights;
//  DynLight dynLights[16];
// } lights; 

