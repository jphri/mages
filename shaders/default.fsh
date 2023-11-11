#version 310 es
#extension GL_OES_shader_io_blocks : require
precision mediump float;

uniform sampler2D u_ImageTexture[8];

in VS_OUT {
	vec2 texcoord;
	vec4 color;
	flat int sprite_type;
	flat vec4 clip_region;
	vec2 position;
} fs_in;

out vec4 out_FragColor;

vec4 getTextureCoords(int id, vec2 texcoord) {
	#define ID(I) if(id == I) return texture(u_ImageTexture[I], texcoord);
	ID(0);
	ID(1);
	ID(2);
	ID(3);
	ID(4);
	ID(5);
	ID(6);
	ID(7);
	#undef ID
	return vec4(0.0, 0.0, 0.0, 1.0);
}

float rectdist(vec2 pixel, vec2 position, vec2 half_size)
{
	vec2 v = abs(pixel - position) - half_size;
	return max(v.x, v.y);
}

void main() {
	vec2 clip_position = fs_in.clip_region.xy;
	vec2 clip_size     = fs_in.clip_region.zw;
	float d = rectdist(fs_in.position, clip_position, clip_size);
	d = min(1.0, max(0.0, 1.0 - d));
	out_FragColor = fs_in.color * getTextureCoords(fs_in.sprite_type, fs_in.texcoord) * vec4(1.0, 1.0, 1.0, d);
}
