#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "vecmath.h"
#include "physics.h"
#include "graphics.h"
#include "util.h"
#include "map.h"

Map *
map_alloc(int w, int h)
{
	Map *map;
	map = calloc(1, sizeof(*map) + sizeof(map->tiles[0]) * w * h * 64);
	map->w = w;
	map->h = h;
	map->tiles = (int*)(map + 1);
	map->collision = NULL;
	return map;
}

Map *
map_load(const char *file) 
{
	char *line;
	FILE *fp = fopen(file, "r");
	Map *map = NULL;

	if(!fp) {
		fprintf(stderr, "failed load file %s\n", file);
		return NULL;
	}
	
	while((line = readline(fp))) {
		StrView tokenview = to_strview(line);
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
		} else {
			char *s = strview_str(word);
			printf("unknown command %s\n", s);
			free(s);
		}

		free(line);
	}

	return map;

error_load:
	if(map)
		free(map);
	return NULL;
}

void
map_free(Map *map)
{
	for(CollisionData *c = map->collision, *next; c; c = next) {
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

	*out_data_size = buffer.size;
	return buffer.data;
}

void
map_set_gfx_scene(Map *map) 
{
	/* i really need to improve the map layout lol */
	for(size_t i = 0; i < 64; i++) {
		int layer_index = i * map->w * map->h;
		gfx_scene_set_tilemap(i, TERRAIN_NORMAL, map->w, map->h, &map->tiles[layer_index]);
	}
}

void
map_set_phx_scene(Map *map) 
{
	for(CollisionData *c = map->collision; c; c = c->next) {
		BodyID body = phx_new();
		vec2_dup(phx_data(body)->position,  c->position);
		vec2_dup(phx_data(body)->half_size, c->half_size);
		
		phx_data(body)->collision_layer = PHX_LAYER_MAP_BIT;
		phx_data(body)->solve_layer     = PHX_LAYER_MAP_BIT;
		phx_data(body)->collision_mask  = 0;
		phx_data(body)->solve_mask      = 0;
		phx_data(body)->user_data       = 0;
		phx_data(body)->no_update       = true;
		phx_data(body)->is_static       = true;
		phx_data(body)->mass            = 0.0;
		phx_data(body)->restitution     = 0.0;
	}
}

