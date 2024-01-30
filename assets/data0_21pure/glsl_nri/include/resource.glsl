layout(set = 0, binding = 0) uniform Scene {
    float shaderTime;
} set_scene;

layout(set = 0, binding = 1) uniform View {
    mat4 viewOrigin;
    mat4 viewAxis;
    ivec4 viewport;
} set_view;


struct DynamicLights{
    float4 lightDiffuse;
    float3 position;
    float invRadius;
};

layout(set = 0, binding = 2) uniform Object {
    mat4 mvp;
    mat4 mv;
    DynamicLights dynamicLights[NUM_DLIGHTS];
} set_object;
