layout(set = DESCRIPTOR_OBJECT_SET, binding = 4) uniform DefaultQ3ShaderCB {
    vec4 textureParam;
    vec2 zRange;
    mat4 genTexMatrix;
    vec3 lightstyleColor[4];
    vec3 wallColor;
    vec3 floorColor;
} pass;

