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

static int size_command(Map **map, StrView *tokenview);
static int tile_command(Map **map, StrView *tokenview);
static int collision_command(Map **map, StrView *tokenview);
static int thing_command(Map **map, StrView *tokenview);

static int new_thing_command(Map **map, StrView *tokenview);
static int thing_position_command(Map **map, StrView *tokenview);

static ThingFunc thing_pc[LAST_THING] = {
	[THING_PLAYER] = thing_player,
	[THING_DUMMY]  = thing_dummy
};

static struct {
	const char *name;
	int (*process)(Map **map, StrView *tokenview);
} commands[] = {
	{ "size", size_command },
	{ "tile", tile_command },
	{ "collision",  collision_command },
	{ "thing", thing_command },
	{ "new_thing", new_thing_command },
	{ "thing_position", thing_position_command }
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

		for(size_t i = 0; i < LENGTH(commands); i++) {
			if(strview_cmp(word, commands[i].name) == 0) {
				if(commands[i].process(&map, &tokenview))
					goto error_load;
				else
					goto continue_loading;
			}
		}
		char *s = strview_str(word);
		printf("unknown command %s\n", s);
		free(s);

continue_loading:
		continue;
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
		Body *body = phx_new();
		vec2_dup(body->position,  VEC2_DUP(c->position));
		vec2_dup(body->half_size, VEC2_DUP(c->half_size));
		
		body->collision_layer = PHX_LAYER_MAP_BIT;
		body->solve_layer     = PHX_LAYER_MAP_BIT;
		body->collision_mask  = 0;
		body->solve_mask      = 0;
		body->entity          = NULL;
		body->no_update       = false;
		body->is_static       = true;
		body->mass            = 0.0;
		body->restitution     = 0.0;
	}
}

void 
map_set_ent_scene(Map *map)
{
	for(Thing *c = map->things; c; c = c->next)
		if(thing_pc[c->type])
			thing_pc[c->type](c);
}

int
size_command(Map **map, StrView *tokenview)
{
	int w, h;
	if(!strview_int(strview_token(tokenview, " "), &w))
		return 1;

	if(!strview_int(strview_token(tokenview, " "), &h))
		return 1;

	*map = map_alloc(w, h);
	return 0;
}

int
tile_command(Map **map, StrView *tokenview)
{
	int tile_id, tile;
	if(!strview_int(strview_token(tokenview, " "), &tile_id))
		return 1;

	if(!strview_int(strview_token(tokenview, " "), &tile))
		return 1;

	(*map)->tiles[tile_id] = tile;
	return 0;
}

int
collision_command(Map **map, StrView *tokenview)
{
	CollisionData *data = malloc(sizeof(*data));
	if(!strview_float(strview_token(tokenview, " "), &data->position[0]))
		return 1;

	if(!strview_float(strview_token(tokenview, " "), &data->position[1]))
		return 1;

	if(!strview_float(strview_token(tokenview, " "), &data->half_size[0]))
		return 1;

	if(!strview_float(strview_token(tokenview, " "), &data->half_size[1]))
		return 1;

	data->next = (*map)->collision;
	(*map)->collision = data;

	return 0;
}

int
thing_command(Map **map, StrView *tokenview)
{
	Thing *data = malloc(sizeof(*data));

	if(!strview_int(strview_token(tokenview, " "), &data->type))
		return 1;

	if(!strview_float(strview_token(tokenview, " "), &data->position[0]))
		return 1;

	if(!strview_float(strview_token(tokenview, " "), &data->position[1]))
		return 1;

	data->next = (*map)->things;
	data->prev = NULL;
	if((*map)->things)
		(*map)->things->prev = data;

	(*map)->things = data;
	return 0;
}

int
new_thing_command(Map **map, StrView *tokenview)
{
	Thing *data = malloc(sizeof(*data));

	if(!strview_int(strview_token(tokenview, " "), &data->type))
		return 1;
	
	data->next = (*map)->things;
	data->prev = NULL;
	if((*map)->things)
		(*map)->things->prev = data;

	(*map)->things = data;

	return 0;
}

int
thing_position_command(Map **map, StrView *tokenview)
{
	Thing *thing = (*map)->things;
	
	if(!strview_float(strview_token(tokenview, " "), &thing->position[0]))
		return 1;

	if(!strview_float(strview_token(tokenview, " "), &thing->position[1]))
		return 1;

	return 0;
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

