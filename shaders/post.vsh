#version 310 es
#extension GL_OES_shader_io_blocks : require
precision mediump float;

in vec2 v_Position;
in vec2 v_Texcoord;

out vec2 in_Texcoord;

void
main()
{
	gl_Position = vec4(v_Position, 0.0, 1.0);
	in_Texcoord = v_Texcoord;
}
