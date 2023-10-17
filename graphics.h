typedef float vec2[2];
typedef float vec4[4];

typedef struct {
	vec2 position;
	vec2 half_size;
	vec2 sprite_id;
	vec4 color;
	float rotation;
} Sprite;

void gfx_init();
void gfx_end();

void gfx_reset();
void gfx_draw_sprite(Sprite *sprite);
void gfx_render(int w, int h);
