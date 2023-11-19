#include <string.h>
#include <glad/gles2.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "vecmath.h"
#include "util.h"
#include "glutil.h"
#include "graphics.h"

typedef struct {
	SceneObjectID next, prev;
	SceneObjectID next_layer, prev_layer;
	SceneObjectType type;
	int layer;
	bool dead;
	
	union {
		SceneSprite sprite;
		SceneText   text;
		SceneAnimatedSprite anim;
	} data;
} SceneObjectNode;

#define SYS_ID_TYPE   SceneObjectID
#define SYS_NODE_TYPE SceneObjectNode
#include "system.h"

typedef struct {
	int frame_count;
	vec2 *frames;
} AnimationData;

#define DEFINE_ANIMATION(ANIMATION_NAME, ...) [ANIMATION_NAME] = { \
		.frame_count = sizeof((vec2[]){ __VA_ARGS__ })/sizeof(vec2), \
		.frames = (vec2[]){ __VA_ARGS__ } \
	}

static AnimationData animations[LAST_ANIMATION] = {
	DEFINE_ANIMATION(ANIMATION_NULL, { 0.0, 0.0 }),
	DEFINE_ANIMATION(ANIMATION_PLAYER_MOVEMENT,
		{ 0.0, 0.0 }, 
		{ 1.0, 0.0 }
	)
};

static inline int calculate_frame_animation(SceneAnimatedSprite *sprite)
{
	int frame = (int)(sprite->time * sprite->fps);
	return frame % animations[sprite->animation].frame_count;
}

static SceneObjectID   layer_objects[SCENE_LAYERS];
static GraphicsTileMap layer_tmaps[SCENE_LAYERS];
static uint64_t        layer_tmap_set;

static inline void insert_object_layer(SceneObjectID id) 
{
	int layer_id = _sys_node(id)->layer;
	
	_sys_node(id)->next_layer = layer_objects[layer_id];
	_sys_node(id)->prev_layer = 0;

	if(layer_objects[layer_id])
		_sys_node(layer_objects[layer_id])->prev_layer = id;
	layer_objects[layer_id] = id;
}

static inline void remove_object_layer(SceneObjectID id) 
{
	SceneObjectID next = _sys_node(id)->next_layer,
				  prev = _sys_node(id)->prev_layer;
	int lay            = _sys_node(id)->layer;

	if(next) _sys_node(next)->prev_layer = _sys_node(id)->prev_layer;
	if(prev) _sys_node(prev)->next_layer = _sys_node(id)->next_layer;
	if(layer_objects[lay] == id) layer_objects[lay] = next;
}

static SceneObjectID new_object(SceneObjectType type, int layer) 
{
	SceneObjectID id = _sys_new();
	_sys_node(id)->type = type;
	_sys_node(id)->layer = layer;
	insert_object_layer(id);

	return id;
}

static void del_object(SceneObjectID id) 
{
	_sys_del(id);
}

static void _sys_int_cleanup(SceneObjectID id) 
{
	remove_object_layer(id);
}

void
gfx_scene_setup()
{
	_sys_init();
	memset(layer_objects, 0, sizeof(layer_objects));
}

void
gfx_scene_cleanup()
{
	_sys_deinit();
}

void
gfx_scene_reset()
{
	layer_tmap_set = 0;
	for(int i = 0; i < SCENE_LAYERS; i++) {
		layer_objects[i] = 0;
		gfx_tmap_free(&layer_tmaps[i]);
	}
	_sys_reset();
}

void 
gfx_scene_draw()
{
	_sys_cleanup();
	for(int i = 0; i < SCENE_LAYERS; i++) {
		SceneSpriteID object_id = layer_objects[i];
		gfx_draw_begin(layer_tmap_set & (1 << i) ? &layer_tmaps[i] : NULL);
		while(object_id) {
			SceneSprite *ss = gfx_scene_spr(object_id);
			SceneText *tt = gfx_scene_text(object_id);
			SceneAnimatedSprite *as = gfx_scene_animspr(object_id);
			int frame;

			switch(_sys_node(object_id)->type) {
			case SCENE_OBJECT_SPRITE:
				ss->sprite.type = TEXTURE_ENTITIES;
				ss->sprite.clip_region[0] = ss->sprite.position[0];
				ss->sprite.clip_region[1] = ss->sprite.position[1];
				ss->sprite.clip_region[2] = 100000.0;
				ss->sprite.clip_region[3] = 100000.0;
				gfx_draw_sprite(&ss->sprite);
				break;
			case SCENE_OBJECT_TEXT:
				gfx_draw_font(TEXTURE_FONT_CELLPHONE,
						tt->position,
						tt->char_size,
						tt->color,
						(vec4){ 0, 0, 100000, 100000 },
						"%s",
						tt->text_ptr);
				break;
			case SCENE_OBJECT_ANIMATED_SPRITE:
				frame = calculate_frame_animation(as);
				vec2_add(as->sprite.sprite_id, as->sprite_id, animations[as->animation].frames[frame]);
				as->sprite.type = TEXTURE_ENTITIES;
				as->sprite.clip_region[0] = as->sprite.position[0];
				as->sprite.clip_region[1] = as->sprite.position[1];
				as->sprite.clip_region[2] = 100000.0;
				as->sprite.clip_region[3] = 100000.0;
				gfx_draw_sprite(&as->sprite);
				break;
			default: 
				assert(0 && "invalid object type");
			}
			object_id = _sys_node(object_id)->next_layer;
		}
		gfx_draw_end();
	}
}

SceneSpriteID  
gfx_scene_new_obj(int layer, SceneObjectType type)
{
	return new_object(type, layer);
}

void 
gfx_scene_del_obj(SceneObjectID obj_id) 
{
	del_object(obj_id);
}

void
gfx_scene_update(float delta)
{
	for(int i = 0; i < SCENE_LAYERS; i++) {
		SceneSpriteID object_id = layer_objects[i];

		while(object_id) {
			SceneAnimatedSprite *as = gfx_scene_animspr(object_id);

			switch(_sys_node(object_id)->type) {
			case SCENE_OBJECT_ANIMATED_SPRITE:
				as->time += delta;
			default:
				do {} while(0);
			}

			object_id = _sys_node(object_id)->next_layer;
		}
	}
}

SceneSprite *
gfx_scene_spr(SceneSpriteID spr_id)
{
	return &_sys_node(spr_id)->data.sprite;
}

SceneText *
gfx_scene_text(SceneTextID text_id)
{
	return &_sys_node(text_id)->data.text;
}

SceneAnimatedSprite *
gfx_scene_animspr(SceneAnimatedSpriteID text_id)
{
	return &_sys_node(text_id)->data.anim;
}

void
gfx_scene_set_tilemap(int layer, TextureAtlas atlas, int w, int h, int *data) 
{
	if(layer_tmap_set & (1 << layer))
		gfx_tmap_free(&layer_tmaps[layer]);
	layer_tmaps[layer] = gfx_tmap_new(atlas, w, h, data);
	layer_tmap_set |= (1 << layer);
}
