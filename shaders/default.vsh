#version 330 core

uniform mat3 u_View;

in vec2 v_Position; 
in vec2 v_Texcoord;
in vec4 v_Color;

out VS_OUT {
	vec2 texcoord;
	vec4 color;
} vs_out;

void main() {
	gl_Position     = vec4(u_View * vec3(v_Position, 1.0), 1.0);
	vs_out.texcoord = v_Texcoord;
	vs_out.color    = v_Color;
}
