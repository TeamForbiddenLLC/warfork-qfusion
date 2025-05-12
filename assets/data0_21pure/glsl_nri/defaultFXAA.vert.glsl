layout(location = 0) out vec2 v_TexCoord;

void main(void)
{
  v_TexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(v_TexCoord * vec2(2, -2) + vec2(-1, 1), 0, 1);
}

