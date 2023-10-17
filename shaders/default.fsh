#version 330 core

in VS_OUT {
	vec2 texcoord;
	vec4 color;
} fs_in;

out vec4 out_FragColor;

void main() {
	out_FragColor = fs_in.color;
}
