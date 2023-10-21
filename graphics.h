typedef float vec2[2];
typedef float vec4[4];

#define TILE_SIZE    16
#define SCENE_LAYERS 64

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


void gfx_draw_begin(GraphicsTileMap *tmap);
void gfx_draw_sprite(Sprite *sprite);
void gfx_draw_end();

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
Sprite        *gfx_scene_spr_data(SceneSpriteID spr_id);
void gfx_scene_set_tilemap(int layer, TextureAtlas atlas, int w, int h, int *data);
