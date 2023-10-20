#version 330 core

uniform mat3 u_Projection;
uniform mat3 u_View;
uniform vec2 u_SpriteCR;

in vec2 v_Position; 
in vec2 v_Texcoord;

in vec2 v_InstPosition;
in vec2 v_InstSize; 
in vec2 v_InstSpriteID;
in float v_InstRotation;
in vec4 v_InstColor;

out VS_OUT {
	vec2 texcoord;
	vec4 color;
} vs_out;

void main() {
	mat3 rotation = mat3(
		cos(v_InstRotation), -sin(v_InstRotation), 0.0,
		sin(v_InstRotation), cos(v_InstRotation), 0.0,
		0.0, 0.0, 1.0
	);
	
	vec3 position = (rotation * vec3(v_Position, 1.0)) * vec3(v_InstSize, 1.0) + vec3(v_InstPosition, 0.0);
	gl_Position     = vec4(u_Projection * u_View * position, 1.0);
	vs_out.texcoord = (v_Texcoord + v_InstSpriteID) / u_SpriteCR;
	vs_out.color    = v_InstColor;
}
