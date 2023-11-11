#version 310 es
#extension GL_OES_shader_io_blocks : require
precision mediump float;

uniform sampler2D u_AlbedoTexture;

in vec2 in_Texcoord;
out vec4 out_FragColor;

void
main()
{
	out_FragColor = texture(u_AlbedoTexture, in_Texcoord);
}
