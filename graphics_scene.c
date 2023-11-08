#include <string.h>
#include <GL/glew.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

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
	} data;
} SceneObjectNode;

#define SYS_ID_TYPE   SceneObjectID
#define SYS_NODE_TYPE SceneObjectNode
#include "system.h"

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
gfx_scene_draw()
{
	_sys_cleanup();
	for(int i = 0; i < SCENE_LAYERS; i++) {
		SceneSpriteID object_id = layer_objects[i];
		gfx_draw_begin(layer_tmap_set & (1 << i) ? &layer_tmaps[i] : NULL);
		while(object_id) {
			SceneSprite *ss = gfx_scene_spr_data(object_id);
			switch(_sys_node(object_id)->type) {
			case SCENE_OBJECT_SPRITE:
				ss->sprite.type = TEXTURE_ENTITIES;
				ss->sprite.clip_region[0] = ss->sprite.position[0];
				ss->sprite.clip_region[1] = ss->sprite.position[1];
				ss->sprite.clip_region[2] = 100.0;
				ss->sprite.clip_region[3] = 100.0;
				gfx_draw_sprite(&ss->sprite);
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
gfx_scene_new_spr(int layer_objects)
{
	return new_object(SCENE_OBJECT_SPRITE, layer_objects);
}

void 
gfx_scene_del_spr(SceneSpriteID spr_id)
{
	del_object(spr_id);
}

SceneSprite *
gfx_scene_spr_data(SceneSpriteID spr_id)
{
	return &_sys_node(spr_id)->data.sprite;
}

void
gfx_scene_set_tilemap(int layer, TextureAtlas atlas, int w, int h, int *data) 
{
	if(layer_tmap_set & (1 << layer))
		gfx_tmap_free(&layer_tmaps[layer]);
	layer_tmaps[layer] = gfx_tmap_new(atlas, w, h, data);
	layer_tmap_set |= (1 << layer);
}
