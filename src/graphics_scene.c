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

typedef struct SceneObjectPrivData SceneObjectPrivData;
struct SceneObjectPrivData {
	SceneObjectPrivData *next, *prev;
	SceneObjectPrivData *next_layer, *prev_layer;
	SceneObjectType type;
	int layer;
	union {
		SceneSprite sprite;
		SceneText   text;
		SceneAnimatedSprite anim;
		SceneLine line;
		SceneTiles tiles;
	} data;
};

typedef struct {
	SpriteType type;
	int     sprite_x, sprite_y;
} Frame;

typedef struct {
	int frame_count;
	Frame *frames;
} AnimationData;

typedef void (*ObjectDel)(SceneObject *obj);

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

static ObjectDel del_functions[LAST_SCENE_OBJECT_TYPE] = {
	0
};

static inline Frame *calculate_frame_animation(SceneAnimatedSprite *sprite)
{
	int    frame = (int)(sprite->time * sprite->fps);
	       frame %= animations[sprite->animation].frame_count;
	return &animations[sprite->animation].frames[frame];
}

static SceneObjectPrivData   *layer_objects[SCENE_LAYERS];
static ObjectPool objects;

#define PRIVDATA(ID) ((SceneObjectPrivData*)objalloc_data(&objects, ID))
#define OBJECT_DATA(ID) ((SceneObjectPrivData*)objalloc_data(&objects, ID))

static inline void insert_object_layer(SceneObjectPrivData *object) 
{
	int layer_id = object->layer;
	
	object->next_layer = layer_objects[layer_id];
	object->prev_layer = NULL;

	if(layer_objects[layer_id])
		layer_objects[layer_id]->prev_layer = object;
	layer_objects[layer_id] = object;
}

static inline void remove_object_layer(SceneObjectPrivData *object) 
{
	SceneObjectPrivData *next = object->next_layer,
				  *prev = object->prev_layer;
	int lay            = object->layer;

	if(next) next->prev_layer = object->prev_layer;
	if(prev) prev->next_layer = object->next_layer;
	if(layer_objects[lay] == object) layer_objects[lay] = next;
}

static SceneObjectPrivData *
new_object(SceneObjectType type, int layer) 
{
	SceneObjectPrivData *object = objpool_new(&objects);
	object->type = type;
	object->layer = layer;
	insert_object_layer(object);

	return object;
}

static void del_object(SceneObjectPrivData *object) 
{
	objpool_free(object);
}

static void 
cleanup_callback(ObjectPool *pool, void *ptr) 
{
	(void)pool;
	SceneObjectPrivData *object = ptr;
	if(del_functions[object->type])
		del_functions[object->type](&object->data);
	remove_object_layer(ptr);
}

void
gfx_scene_setup(void)
{
	objpool_init(&objects, sizeof(SceneObjectPrivData), DEFAULT_ALIGNMENT);
	objects.clean_cbk = cleanup_callback;
	memset(layer_objects, 0, sizeof(layer_objects));
}

void
gfx_scene_reset(void)
{
	for(SceneObjectPrivData *obj = objpool_begin(&objects);
		obj;
		obj = objpool_next(obj))
	{
		objpool_free(obj);
	}
	objpool_clean(&objects);
}

void 
gfx_scene_draw(void)
{
	Rectangle tiles_rect, graphics_rect;

	objpool_clean(&objects);
	gfx_begin();
	for(int i = 0; i < SCENE_LAYERS; i++) {
		SceneObjectPrivData *object_id = layer_objects[i];
		while(object_id) {
			TextureStamp stamp;
			Frame *frame;

			switch(object_id->type) {
			case SCENE_OBJECT_TILES:
				graphics_rect = gfx_window_rectangle();

				gfx_world_to_pixel(object_id->data.tiles.position, tiles_rect.position);
				gfx_world_scale_to_pixel_scale(object_id->data.tiles.half_size, tiles_rect.half_size);
				vec2_add(graphics_rect.half_size, graphics_rect.half_size, tiles_rect.half_size);
				if(rect_contains_point(&graphics_rect, tiles_rect.position)) {
					stamp = get_sprite(object_id->data.tiles.type, object_id->data.tiles.sprite_x, object_id->data.tiles.sprite_y);
					gfx_push_texture_rect(&stamp, object_id->data.tiles.position, object_id->data.tiles.half_size, object_id->data.tiles.uv_scale, 0, (vec4){ 1.0, 1.0, 1.0, 1.0 });
				}
				break;
			case SCENE_OBJECT_SPRITE:
				stamp = get_sprite(object_id->data.sprite.type, object_id->data.sprite.sprite_x, object_id->data.sprite.sprite_y);
				gfx_push_texture_rect(&stamp, object_id->data.sprite.position, object_id->data.sprite.half_size, object_id->data.sprite.uv_scale, object_id->data.sprite.rotation, object_id->data.sprite.color);
				break;
			case SCENE_OBJECT_TEXT:
				gfx_push_font2(FONT_ROBOTO,
						object_id->data.text.position,
						object_id->data.text.char_size[0],
						object_id->data.text.color,
						"%s",
						object_id->data.text.text_ptr);
				break;
			case SCENE_OBJECT_ANIMATED_SPRITE:
				frame = calculate_frame_animation(&object_id->data.anim);
				stamp = get_sprite(frame->type, frame->sprite_x, frame->sprite_y);
				gfx_push_texture_rect(&stamp, object_id->data.anim.position, object_id->data.anim.half_size, (vec2){ 1.0, 1.0 }, object_id->data.anim.rotation, object_id->data.anim.color);
				break;
			case SCENE_OBJECT_LINE:
				gfx_push_line(
					object_id->data.line.p1,
					object_id->data.line.p2,
					object_id->data.line.thickness,
					object_id->data.line.color);
				break;

			default: 
				assert(0 && "invalid object type");
			}
			object_id = object_id->next_layer;
		}
	}
	gfx_flush();
	gfx_end();
}

SceneObject *  
gfx_scene_new_obj(int layer, SceneObjectType type)
{
	SceneObjectPrivData *obj = new_object(type, layer);
	return &obj->data;
}

void 
gfx_scene_del_obj(SceneObject *obj) 
{
	del_object(CONTAINER_OF(obj, SceneObjectPrivData, data));
}

void
gfx_scene_update(float delta)
{
	for(int i = 0; i < SCENE_LAYERS; i++) {
		SceneObjectPrivData *object_id = layer_objects[i];

		while(object_id) {
			switch(object_id->type) {
			case SCENE_OBJECT_ANIMATED_SPRITE:
				object_id->data.anim.time += delta;
			default:
				do {} while(0);
			}

			object_id = object_id->next_layer;
		}
	}
}

