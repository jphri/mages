#version 310 es
#extension GL_OES_shader_io_blocks : require
precision mediump float;

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
in vec2  v_InstPosition;
in vec2  v_InstSize; 
in vec2  v_InstTexPosition;
in vec2  v_InstTexSize;
in float v_InstRotation;
in vec4  v_InstColor;
in int   v_InstSpriteType;
in vec4  v_ClipRegion;
in vec2  v_InstUVScale;

out VS_OUT {
	vec2 uv;
	vec2 texpos;
	vec2 texsize;
	vec4 color;
	flat int sprite_type;
	flat vec4 clip_region;
	vec2 position;
} vs_out;

void main() {
	mat4 rotation = mat4(
		cos(v_InstRotation), -sin(v_InstRotation), 0.0, 0.0,
		sin(v_InstRotation), cos(v_InstRotation), 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0
	);
	
	vec4 position = (rotation * (vec4(v_Position, 0.0, 1.0) * vec4(v_InstSize, 1.0, 1.0)) + vec4(v_InstPosition, 0.0, 0.0));
	gl_Position     = u_Projection * u_View * position;
	vs_out.uv     = v_Texcoord * v_InstUVScale;
	vs_out.texpos = v_InstTexPosition;
	vs_out.texsize = v_InstTexSize;
	vs_out.color    = v_InstColor;
	vs_out.sprite_type = v_InstSpriteType;
	vs_out.position = (u_View * position).xy;
	vs_out.clip_region = v_ClipRegion;
}

