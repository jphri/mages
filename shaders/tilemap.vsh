#version 330 core

layout(std140) uniform u_TransformBlock 
{
	mat4 u_Projection;
	mat4 u_View;
};

layout(std140) uniform u_SpriteDataBlock 
{
	vec2 u_SpriteInvColRow[32];
};

in vec2 v_Position; 
in vec2 v_Texcoord;

in vec2 v_InstPosition;
in vec2 v_InstSpriteID;

out VS_OUT {
	vec2 texcoord;
	vec4 color;
	flat int sprite_type;
	flat vec4 clip_region;
	vec2 position;
} vs_out;

void main() {
	vec3 position   = (vec3((v_Position + 1.0) / 2.0, 1.0) + vec3(v_InstPosition, 0.0));
	vec4 view_position = u_View * vec4(position.xy, 0.0, 1.0);

	gl_Position     = u_Projection * view_position;
	vs_out.texcoord = (v_Texcoord + v_InstSpriteID) * u_SpriteInvColRow[1];
	vs_out.color    = vec4(1.0);
	vs_out.sprite_type = 1;
	vs_out.position = view_position.xy;
	vs_out.clip_region = vec4(view_position.xy, 10000, 10000);
}
