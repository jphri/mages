#include <string.h>
#include <glad/gles2.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "game_objects.h"
#include "vecmath.h"
#include "util.h"
#include "glutil.h"
#include "graphics.h"
#include "global.h"

typedef struct {
	SceneObjectID next, prev;
	SceneObjectID next_layer, prev_layer;
	SceneObjectType type;
	int layer;
	union {
		SceneSprite sprite;
		SceneText   text;
		SceneAnimatedSprite anim;
	} data;
} SceneObjectPrivData;

typedef struct {
	SpriteType type;
	int     sprite_x, sprite_y;
} Frame;

typedef struct {
	int frame_count;
	Frame *frames;
} AnimationData;

#define DEFINE_ANIMATION(ANIMATION_NAME, ...) [ANIMATION_NAME] = { \
		.frame_count = sizeof((Frame[]){ __VA_ARGS__ })/sizeof(Frame), \
		.frames = (Frame[]){ __VA_ARGS__ } \
	}

static AnimationData animations[LAST_ANIMATION] = {
	DEFINE_ANIMATION(ANIMATION_NULL, { SPRITE_UI, 0, 0 }),
	DEFINE_ANIMATION(ANIMATION_PLAYER_MOVEMENT,
		{ SPRITE_ENTITIES, 0, 0 }, 
		{ SPRITE_ENTITIES, 1, 0 }
	),

	DEFINE_ANIMATION(ANIMATION_PLAYER_IDLE,
		{ SPRITE_ENTITIES, 0, 0 },
	)
};

static inline Frame *calculate_frame_animation(SceneAnimatedSprite *sprite)
{
	int    frame = (int)(sprite->time * sprite->fps);
	       frame %= animations[sprite->animation].frame_count;
	return &animations[sprite->animation].frames[frame];
}

static SceneObjectID   layer_objects[SCENE_LAYERS];
static GraphicsTileMap layer_tmaps[SCENE_LAYERS];
static uint64_t        layer_tmap_set;
static ObjectAllocator objects;

#define PRIVDATA(ID) ((SceneObjectPrivData*)objalloc_data(&objects, ID))
#define OBJECT_DATA(ID) ((SceneObjectPrivData*)objalloc_data(&objects, ID))

static inline void insert_object_layer(SceneObjectID id) 
{
	int layer_id = PRIVDATA(id)->layer;
	
	PRIVDATA(id)->next_layer = layer_objects[layer_id];
	PRIVDATA(id)->prev_layer = 0;

	if(layer_objects[layer_id])
		PRIVDATA(layer_objects[layer_id])->prev_layer = id;
	layer_objects[layer_id] = id;
}

static inline void remove_object_layer(SceneObjectID id) 
{
	SceneObjectID next = PRIVDATA(id)->next_layer,
				  prev = PRIVDATA(id)->prev_layer;
	int lay            = PRIVDATA(id)->layer;

	if(next) PRIVDATA(next)->prev_layer = PRIVDATA(id)->prev_layer;
	if(prev) PRIVDATA(prev)->next_layer = PRIVDATA(id)->next_layer;
	if(layer_objects[lay] == id) layer_objects[lay] = next;
}

static SceneObjectID new_object(SceneObjectType type, int layer) 
{
	SceneObjectID id = objalloc_alloc(&objects);
	PRIVDATA(id)->type = type;
	PRIVDATA(id)->layer = layer;
	insert_object_layer(id);

	return id;
}

static void del_object(SceneObjectID id) 
{
	objalloc_free(&objects, id);
}

static void cleanup_callback(ObjectAllocator *alloc, ObjectID id) 
{
	(void)alloc;
	remove_object_layer(id);
}

void
gfx_scene_setup(void)
{
	objalloc_init_allocator(&objects, sizeof(SceneObjectPrivData), cache_aligned_allocator());
	objects.clean_cbk = cleanup_callback;
	memset(layer_objects, 0, sizeof(layer_objects));
}

void
gfx_scene_reset(void)
{
	layer_tmap_set = 0;
	for(int i = 0; i < SCENE_LAYERS; i++) {
		layer_objects[i] = 0;
		gfx_tmap_free(&layer_tmaps[i]);
	}
	objalloc_reset(&objects);
}

void 
gfx_scene_draw(void)
{
	objalloc_clean(&objects);
	for(int i = 0; i < SCENE_LAYERS; i++) {
		SceneSpriteID object_id = layer_objects[i];
		gfx_draw_begin(layer_tmap_set & (1 << i) ? &layer_tmaps[i] : NULL);
		while(object_id) {
			SceneSprite *ss = gfx_scene_spr(object_id);
			SceneText *tt = gfx_scene_text(object_id);
			SceneAnimatedSprite *as = gfx_scene_animspr(object_id);
			TextureStamp stamp;
			Frame *frame;

			switch(PRIVDATA(object_id)->type) {
			case SCENE_OBJECT_SPRITE:
				stamp = get_sprite(ss->type, ss->sprite_x, ss->sprite_y);
				gfx_draw_texture_rect(&stamp, ss->position, ss->half_size, ss->rotation, ss->color);
				break;
			case SCENE_OBJECT_TEXT:
				gfx_draw_font2(FONT_ROBOTO,
						tt->position,
						tt->char_size[0],
						tt->color,
						"%s",
						to_ptr(tt->text_ptr));
				break;
			case SCENE_OBJECT_ANIMATED_SPRITE:
				frame = calculate_frame_animation(as);
				stamp = get_sprite(frame->type, frame->sprite_x, frame->sprite_y);
				gfx_draw_texture_rect(&stamp, as->position, as->half_size, as->rotation, as->color);
				break;
			default: 
				assert(0 && "invalid object type");
			}
			object_id = PRIVDATA(object_id)->next_layer;
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

			switch(PRIVDATA(object_id)->type) {
			case SCENE_OBJECT_ANIMATED_SPRITE:
				as->time += delta;
			default:
				do {} while(0);
			}

			object_id = PRIVDATA(object_id)->next_layer;
		}
	}
}

SceneSprite *
gfx_scene_spr(SceneSpriteID spr_id)
{
	return &OBJECT_DATA(spr_id)->data.sprite;
}

SceneText *
gfx_scene_text(SceneTextID text_id)
{
	return &OBJECT_DATA(text_id)->data.text;
}

SceneAnimatedSprite *
gfx_scene_animspr(SceneAnimatedSpriteID text_id)
{
	return &OBJECT_DATA(text_id)->data.anim;
}

void
gfx_scene_set_tilemap(int layer, SpriteType atlas, int w, int h, int *data) 
{
	if(layer_tmap_set & (1 << layer))
		gfx_tmap_free(&layer_tmaps[layer]);
	layer_tmaps[layer] = gfx_tmap_new(atlas, w, h, data);
	layer_tmap_set |= (1 << layer);
}
