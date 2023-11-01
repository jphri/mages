#version 330 core

layout(std140) uniform u_TransformBlock 
{
	mat4 u_Projection;
	mat4 u_View;
};

in vec2 v_Position; 
in vec4 v_Color;

out VS_OUT {
	vec4 color;
} vs_out;

void main() {
	gl_Position     = u_Projection * u_View * vec4(v_Position, 0.0, 1.0);
	vs_out.color    = v_Color;
}
