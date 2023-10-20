typedef float vec2[2];
typedef float vec4[4];

#define TILE_SIZE 16

typedef enum {
	SPRITE_PLAYER,
	SPRITE_FIRIE,
	LAST_SPRITE
} SpriteType;

typedef enum {
	TEXTURE_PLAYER,
	TEXTURE_FIRIE,
	TERRAIN_NORMAL,
	LAST_TEXTURE_ATLAS
} TextureAtlas;

typedef struct {
	SpriteType sprite_type;
	vec2 position;
	vec2 half_size;
	vec2 sprite_id;
	vec4 color;
	float rotation;
} Sprite;

typedef struct {
	unsigned int vao;
	unsigned int buffer;
	unsigned int count_tiles;
	TextureAtlas terrain;
} GraphicsTileMap;

void gfx_init();
void gfx_end();

void gfx_reset();
void gfx_draw_sprite(Sprite *sprite);
void gfx_render(int w, int h);

GraphicsTileMap  gfx_tmap_new(TextureAtlas terrain, int w, int h, int *data);
void             gfx_tmap_free(GraphicsTileMap *tmap);
void             gfx_tmap_draw(GraphicsTileMap *tmap);

