#include "attributes.glsl"
#include "resource.glsl"

layout(set = DESCRIPTOR_OBJECT_SET, binding = 0) uniform PostProcessingCB post; 

layout(location = 0) out vec2 v_TexCoord;

void main(void)
{
    gl_Position = post.mvp * a_Position;
	v_TexCoord = TextureMatrix2x3Mul(post.textureMatrix, a_TexCoord);
}
