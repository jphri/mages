#version 330 core

uniform sampler2D u_ImageTexture;

in VS_OUT {
	vec2 texcoord;
	vec4 color;
} fs_in;

out vec4 out_FragColor;

void main() {
	out_FragColor = texture2D(u_ImageTexture, fs_in.texcoord) * fs_in.color;
}
