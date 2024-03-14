#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdbool.h>

#include "util.h"
#include "vecmath.h"
#include "game_objects.h"

#define TILE_SIZE    16
#define SCENE_LAYERS 64
#define ENTITY_SCALE 0.5

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
	TEXTURE_FONT_ROBOTO,
	LAST_TEXTURE_ATLAS
} Texture;

typedef enum {
	ANIMATION_NULL,
	ANIMATION_PLAYER_IDLE,
	ANIMATION_PLAYER_MOVEMENT,
	LAST_ANIMATION
} Animation;

typedef enum {
	SCENE_OBJECT_SPRITE,
	SCENE_OBJECT_TEXT,
	SCENE_OBJECT_ANIMATED_SPRITE,
	SCENE_OBJECT_LINE,
	SCENE_OBJECT_TILEMAP,
	LAST_SCENE_OBJECT_TYPE,
} SceneObjectType;

typedef enum {
	FONT_NULL,
	FONT_ROBOTO,
	LAST_FONT
} Font;

typedef struct {
	Texture texture;
	vec2 position;
	vec2 size;
} TextureStamp;

typedef struct {
	SpriteType type;
	int sprite_x, sprite_y;

	float rotation;
	vec2 position;
	vec2 half_size;
	vec4 color;
} SceneSprite;

typedef struct {
	vec2 p1, p2;
	vec4 color;
	float thickness;
} SceneLine;

typedef struct {
	vec2 position;
	vec2 half_size;
	vec4 color;
	float rotation;

	Animation animation;
	float time;
	float fps;
} SceneAnimatedSprite;

typedef struct {
	const char *text_ptr;
	vec2 position;
	vec2 char_size;
	vec4 color;
} SceneText;

typedef struct {
	unsigned int vao;
	unsigned int buffer;
	unsigned int count_tiles;
	SpriteType terrain;
} GraphicsTileMap;

typedef struct {
	GraphicsTileMap tilemap;
} SceneTileMap;

void gfx_init(void);
void gfx_terminate(void);

void gfx_make_framebuffers(int w, int h);
void gfx_clear(void);

void gfx_setup_draw_framebuffers(void);
void gfx_end_draw_framebuffers(void);
void gfx_render_present(void);

void gfx_set_camera(vec2 position, vec2 scale);
void gfx_pixel_to_world(vec2 pixel, vec2 world_out);
void gfx_world_to_pixel(vec2 world, vec2 pixel_out);

void gfx_begin(void);
void gfx_push_clip(vec2 position, vec2 half_size);
void gfx_pop_clip(void);
void gfx_draw_tilemap(GraphicsTileMap *tmap);
void gfx_push_texture_rect(TextureStamp *texture, vec2 position, vec2 size, float rotation, vec4 color);
void gfx_push_font(Font font, vec2 position, float height, vec4 color, StrView utf_text);
void gfx_push_font2(Font font, vec2 position, float height, vec4 color, const char *fmt, ...);
void gfx_push_line(vec2 p1, vec2 p2, float thickness, vec4 color);
void gfx_push_rect(vec2 position, vec2 half_size, float thickness, vec4 color);
void gfx_flush(void);
void gfx_end(void);

void gfx_camera_set_enabled(bool enabled);

GraphicsTileMap  gfx_tmap_new(SpriteType terrain, int w, int h, int *data);
void             gfx_tmap_free(GraphicsTileMap *tmap);
void             gfx_tmap_draw(GraphicsTileMap *tmap);

void gfx_scene_setup(void); 
void gfx_scene_cleanup(void);
void gfx_scene_draw(void);
void gfx_scene_reset(void);

typedef void SceneObject;

SceneObject *gfx_scene_new_obj(int layer, SceneObjectType type);
void         gfx_scene_del_obj(SceneObject *object);
void         gfx_scene_update(float delta);
SceneTileMap *gfx_scene_new_tilemap(int layer, SpriteType terrain, int w, int h, int *data);

TextureStamp get_sprite(SpriteType sprite, int sprite_x, int sprite_y);
TextureStamp *gfx_white_texture(void);

void gfx_sprite_count_rows_cols(SpriteType type, int *rows_out, int *cols_out);
Rectangle gfx_window_rectangle(void);

void gfx_font_size(vec2 out_size, Font font, float height, const char *fmt, ...);
void gfx_font_size_view(vec2 out_size, Font font, float height, StrView view);

int  gfx_debug_draw_count(void);
int  gfx_debug_sprites_rendered(void);
void gfx_debug_reset(void);

#endif
