#version 330 core

uniform sampler2D u_AlbedoTexture;

in vec2 in_Texcoord;

out vec4 out_FragColor;

void
main()
{
	out_FragColor = texture2D(u_AlbedoTexture, in_Texcoord);
}
