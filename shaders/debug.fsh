#version 330 core

in VS_OUT {
	vec4 color;
} fs_in;

out vec4 out_Color;

void main() {
	out_Color = fs_in.color;
}
