#version 310 es
#extension GL_OES_shader_io_blocks : require
precision mediump float;

uniform sampler2D u_ImageTexture;

in VS_OUT {
	vec2 texcoord;
	vec4 color;
} fs_in;
out vec4 out_Color;

void
main()
{
	float alpha = texture(u_ImageTexture, fs_in.texcoord).r;
	out_Color = fs_in.color * vec4(1.0, 1.0, 1.0, alpha);
}
