#version 310 es
#extension GL_OES_shader_io_blocks : require
precision mediump float;

in VS_OUT {
	vec4 color;
} fs_in;

out vec4 out_Color;

void main() {
	out_Color = fs_in.color;
}
