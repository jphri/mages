#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "entity.h"
#include "vecmath.h"
#include "physics.h"
#include "graphics.h"
#include "util.h"
#include "map.h"

typedef void (*ThingFunc)(Thing *c);

static void thing_player(Thing *c);
static void thing_dummy(Thing *c);

static ThingFunc thing_pc[LAST_THING] = {
	[THING_PLAYER] = thing_player,
	[THING_DUMMY]  = thing_dummy
};


Map *
map_alloc(int w, int h)
{
	Map *map;
	map = calloc(1, sizeof(*map) + sizeof(map->tiles[0]) * w * h * 64);
	map->w = w;
	map->h = h;
	map->tiles = (int*)(map + 1);
	map->collision = NULL;
	map->things = NULL;
	return map;
}

Map *
map_load(const char *file) 
{
	FileBuffer fp;
	Map *map = NULL;

	if(fbuf_open(&fp, file, "r", allocator_default()))
		return 0;
	
	while(fbuf_read_line(&fp, '\n') != EOF) {
		StrView tokenview = fbuf_data_view(&fp);
		StrView word = strview_token(&tokenview, " ");

		if(strview_cmp(word, "size") == 0) {
			int w, h;

			if(!strview_int(strview_token(&tokenview, " "), &w))
				goto error_load;

			if(!strview_int(strview_token(&tokenview, " "), &h))
				goto error_load;

			map = map_alloc(w, h);
		} else if(strview_cmp(word, "tile") == 0) {
			int tile_id, tile;
			if(!strview_int(strview_token(&tokenview, " "), &tile_id))
				goto error_load;

			if(!strview_int(strview_token(&tokenview, " "), &tile))
				goto error_load;
			
			map->tiles[tile_id] = tile;
		} else if(strview_cmp(word, "collision") == 0) {
			CollisionData *data = malloc(sizeof(*data));
			if(!strview_float(strview_token(&tokenview, " "), &data->position[0]))
				goto error_load;

			if(!strview_float(strview_token(&tokenview, " "), &data->position[1]))
				goto error_load;

			if(!strview_float(strview_token(&tokenview, " "), &data->half_size[0]))
				goto error_load;

			if(!strview_float(strview_token(&tokenview, " "), &data->half_size[1]))
				goto error_load;

			data->next = map->collision;
			map->collision = data;
		} else if(strview_cmp(word, "thing") == 0) {
			Thing *data = malloc(sizeof(*data));
			
			if(!strview_int(strview_token(&tokenview, " "), &data->type))
				goto error_load;

			if(!strview_float(strview_token(&tokenview, " "), &data->position[0]))
				goto error_load;

			if(!strview_float(strview_token(&tokenview, " "), &data->position[1]))
				goto error_load;

			data->next = map->things;
			map->things = data;
		} else {
			char *s = strview_str(word);
			printf("unknown command %s\n", s);
			free(s);
		}
	}
	fbuf_close(&fp);

	return map;

error_load:
	if(map)
		map_free(map);
	fbuf_close(&fp);
	return NULL;
}

void
map_free(Map *map)
{
	for(CollisionData *c = map->collision, *next; c; c = next) {
		next = c->next;
		free(c);
	}
	for(Thing *c = map->things, *next; c; c = next) {
		next = c->next;
		free(c);
	}
	free(map);
}

char *
map_export(Map *map, size_t *out_data_size)
{
	ArrayBuffer buffer;

	arrbuf_init(&buffer);
	arrbuf_printf(&buffer, "size %d %d\n", map->w, map->h);
	for(int k = 0; k < 64; k++)
		for(int i = 0; i < map->w * map->h; i++) {
			int tile = map->tiles[i + k * map->w * map->h];
			if(tile <= 0)
				continue;
			arrbuf_printf(&buffer, "tile %d %d\n", i + k * map->w * map->h, tile);
		}

	for(CollisionData *c = map->collision; c; c = c->next)
		arrbuf_printf(&buffer, "collision %f %f %f %f\n", c->position[0], c->position[1], c->half_size[0], c->half_size[1]);

	for(Thing *t = map->things; t; t = t->next)
		arrbuf_printf(&buffer, "thing %d %f %f\n", t->type, t->position[0], t->position[1]);

	*out_data_size = buffer.size;
	return buffer.data;
}

void
map_set_gfx_scene(Map *map) 
{
	/* i really need to improve the map layout lol */
	for(size_t i = 0; i < 64; i++) {
		int layer_index = i * map->w * map->h;
		gfx_scene_set_tilemap(i, SPRITE_TERRAIN, map->w, map->h, &map->tiles[layer_index]);
	}
}

void
map_set_phx_scene(Map *map) 
{
	for(CollisionData *c = map->collision; c; c = c->next) {
		BodyID body = phx_new();
		vec2_dup(phx_data(body)->position,  VEC2_DUP(c->position));
		vec2_dup(phx_data(body)->half_size, VEC2_DUP(c->half_size));
		
		phx_data(body)->collision_layer = PHX_LAYER_MAP_BIT;
		phx_data(body)->solve_layer     = PHX_LAYER_MAP_BIT;
		phx_data(body)->collision_mask  = 0;
		phx_data(body)->solve_mask      = 0;
		phx_data(body)->user_data       = 0;
		phx_data(body)->no_update       = false;
		phx_data(body)->is_static       = true;
		phx_data(body)->mass            = 0.0;
		phx_data(body)->restitution     = 0.0;
	}
}

void 
map_set_ent_scene(Map *map)
{
	for(Thing *c = map->things; c; c = c->next)
		if(thing_pc[c->type])
			thing_pc[c->type](c);
}

void
thing_player(Thing *c)
{
	ent_player_new(c->position);
}

void
thing_dummy(Thing *c)
{
	ent_dummy_new(c->position);
}
