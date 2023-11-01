typedef float vec2[2];
typedef float vec4[4];

#define TILE_SIZE    16
#define SCENE_LAYERS 64

typedef enum {
	SPRITE_ENTITIES,
	SPRITE_TERRAIN,
	SPRITE_FONT_CELLPHONE,
	SPRITE_UI,
	LAST_SPRITE
} SpriteType;

typedef enum {
	TEXTURE_ENTITIES,
	TERRAIN_NORMAL,
	TEXTURE_FONT_CELLPHONE,
	TEXTURE_UI,
	LAST_TEXTURE_ATLAS
} TextureAtlas;

typedef struct {
	long unsigned int type;
	float rotation;
	vec2 position;
	vec2 sprite_id;
	vec2 half_size;
	vec4 color;
	vec4 clip_region;
} Sprite;

typedef struct {
	SpriteType type;
	Sprite sprite;
} SceneSprite;

typedef struct {
	unsigned int vao;
	unsigned int buffer;
	unsigned int count_tiles;
	TextureAtlas terrain;
} GraphicsTileMap;

typedef enum {
	SCENE_OBJECT_SPRITE,
	LAST_SCENE_OBJECT_TYPE,
} SceneObjectType;

void gfx_init();
void gfx_end();

void gfx_make_framebuffers(int w, int h);
void gfx_clear_framebuffers();

void gfx_setup_draw_framebuffers();
void gfx_end_draw_framebuffers();
void gfx_render_present();

void gfx_set_camera(vec2 position, vec2 scale);
void gfx_pixel_to_world(vec2 pixel, vec2 world_out);

void gfx_draw_begin(GraphicsTileMap *tmap);
void gfx_draw_sprite(Sprite *sprite);
void gfx_draw_font(TextureAtlas atlas, vec2 position, vec2 char_size, vec4 color, vec4 clip_region, const char *fmt, ...);
void gfx_draw_line(TextureAtlas atlas, vec2 p1, vec2 p2, float thickness, vec4 color, vec4 clip_region);
void gfx_draw_rect(TextureAtlas atlas, vec2 position, vec2 half_size, float thickness, vec4 color, vec4 clip_region);
void gfx_draw_end();

void gfx_begin_scissor(vec2 position, vec2 size);
void gfx_end_scissor();

GraphicsTileMap  gfx_tmap_new(TextureAtlas terrain, int w, int h, int *data);
void             gfx_tmap_free(GraphicsTileMap *tmap);
void             gfx_tmap_draw(GraphicsTileMap *tmap);

void gfx_scene_setup(); 
void gfx_scene_cleanup();
void gfx_scene_draw();

typedef unsigned int SceneObjectID;
typedef unsigned int SceneSpriteID;

SceneSpriteID  gfx_scene_new_spr(int layer);
void           gfx_scene_del_spr(SceneSpriteID spr_id);
SceneSprite   *gfx_scene_spr_data(SceneSpriteID spr_id);
void gfx_scene_set_tilemap(int layer, TextureAtlas atlas, int w, int h, int *data);
