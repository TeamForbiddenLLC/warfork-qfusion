layout(set = DESCRIPTOR_OBJECT_SET, binding = 4) uniform DefaultQ3ShaderCB {
    mat4 genTexMatrix;
    vec4 textureParam;
    vec3 lightstyleColor[4];
    vec3 wallColor;
    float softParticlesScale;
    vec2 zRange;
    vec3 floorColor;
} pass;

