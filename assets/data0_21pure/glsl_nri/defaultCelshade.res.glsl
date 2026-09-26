layout(set = DESCRIPTOR_OBJECT_SET, binding = 4) uniform DefaultCellShadeCB {
	vec3 entityColor;
	mat4 reflectionTexMatrix; // std140 mat3 columns are vec4-aligned, so upload as mat4 and use the upper 3x3
} pass;

