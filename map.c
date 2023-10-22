#include <stdio.h>
#include <stdlib.h>

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

			if(strview_int(strview_token(&tokenview, " "), &w))
				goto error_load;
			if(strview_int(strview_token(&tokenview, " "), &h))
				goto error_load;

			map = calloc(1, sizeof(*map) + sizeof(map->tiles[0]) * w * h * 64);
			map->w = w;
			map->h = h;
		}

		if(strview_cmp(word, "tile") == 0) {
			int tile_id, tile;
			if(strview_int(strview_token(&tokenview, " "), &tile_id))
				goto error_load;
			if(strview_int(strview_token(&tokenview, " "), &tile))
				goto error_load;
			
			map->tiles[tile_id] = tile;
		}
		free(line);
	}

	return map;
error_load:
	if(map)
		free(map);
	return NULL;
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
