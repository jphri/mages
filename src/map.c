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
static void thing_door(Thing *c);
static void thing_world_map(Thing *c);

static void create_sprite(int layer, int x, int y, int w, int h, int tile);
static void find_sprite(int layer, int x, int y, int w, int h, int *tiles);
static void setup_layer(int layers, int w, int h, int *tiles);

static int size_command(Map **map, StrView *tokenview);
static int tile_command(Map **map, StrView *tokenview);
static int collision_command(Map **map, StrView *tokenview);
static int thing_command(Map **map, StrView *tokenview);

static int new_thing_command(Map **map, StrView *tokenview);
static int thing_position_command(Map **map, StrView *tokenview);
static int thing_health_command(Map **map, StrView *tokenview);
static int thing_health_max_command(Map **map, StrView *tokenview);
static int thing_direction_command(Map **map, StrView *tokenview);
static int thing_layer_command(Map **map, StrView *tokenview);

static ThingFunc thing_pc[LAST_THING] = {
	[THING_PLAYER]    = thing_player,
	[THING_DUMMY]     = thing_dummy,
	[THING_DOOR]      = thing_door,
	[THING_WORLD_MAP] = thing_world_map
};

static struct {
	bool position;
	bool health, health_max;
	bool direction;
} relevant_component[] = {
	[THING_PLAYER] = { .position = true },
	[THING_DUMMY] = { .position = true }
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
	{ "thing_position", thing_position_command },
	{ "thing_health", thing_health_command },
	{ "thing_max_health", thing_health_max_command },
	{ "thing_direction", thing_direction_command },
	{ "thing_layer", thing_layer_command },
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
		for(MapBrush *brush = c->brush_list, *next; brush; brush = next) {
			next = brush->next;
			free(brush);
		}
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

	for(Thing *t = map->things; t; t = t->next) {
		arrbuf_printf(&buffer, "new_thing %d\n", t->type);
		if(relevant_component[t->type].position) {
			arrbuf_printf(&buffer, "thing_position %f %f\n", t->position[0], t->position[1]);
		}
		if(relevant_component[t->type].health) {
			arrbuf_printf(&buffer, "thing_health %f\n", t->health);
		}
		if(relevant_component[t->type].health_max) {
			arrbuf_printf(&buffer, "thing_max_health %f\n", t->health_max);
		}
		if(relevant_component[t->type].direction) {
			switch(t->direction) {
			case DIR_UP: arrbuf_printf(&buffer, "thing_direction %s\n", "up"); break;
			case DIR_DOWN: arrbuf_printf(&buffer, "thing_direction %s\n", "down"); break;
			case DIR_LEFT: arrbuf_printf(&buffer, "thing_direction %s\n", "left"); break;
			case DIR_RIGHT: arrbuf_printf(&buffer, "thing_direction %s\n", "right"); break;
			}
		}
	}

	*out_data_size = buffer.size;
	return buffer.data;
}

void
map_set_gfx_scene(Map *map) 
{
	for(size_t i = 0; i < 64; i++) {
		int layer_index = i * map->w * map->h;
		setup_layer(i, map->w, map->h, &map->tiles[layer_index]);
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

int
thing_health_command(Map **map, StrView *tokenview)
{
	Thing *thing = (*map)->things;
	
	if(!strview_float(strview_token(tokenview, " "), &thing->health))
		return 1;

	return 0;
}

int
thing_health_max_command(Map **map, StrView *tokenview)
{
	Thing *thing = (*map)->things;
	if(!strview_float(strview_token(tokenview, " "), &thing->health_max))
		return 1;
	return 0;
}

int
thing_direction_command(Map **map, StrView *tokenview)
{
	Thing *thing = (*map)->things;
	StrView tok = strview_token(tokenview, " ");
	if(strview_cmp(tok, "up") == 0) {
		thing->direction = DIR_UP;
	} else if(strview_cmp(tok, "right") == 0) {
		thing->direction = DIR_RIGHT;
	} else if(strview_cmp(tok, "down") == 0) {
		thing->direction = DIR_DOWN;
	} else if(strview_cmp(tok, "left") == 0) {
		thing->direction = DIR_LEFT;
	} else {
		return 1;
	}
	return 0;
}

int
thing_layer_command(Map **map, StrView *tokenview)
{
	Thing *thing = (*map)->things;
	if(!strview_int(strview_token(tokenview, " "), &thing->layer)) {
		return 1;
	}
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

void
thing_door(Thing *c)
{
	ent_door_new(c->position, c->direction);
}

void
thing_world_map(Thing *c)
{
	SceneTiles *tiles;
	SceneAnimatedTiles *anim_tiles;
	int rows, cols;

	for(MapBrush *brush = c->brush_list_end; brush; brush = brush->prev) {
		switch(brush->tile) {
		case 5:
			anim_tiles = gfx_scene_new_obj(c->layer, SCENE_OBJECT_ANIMATED_TILES);
			vec2_dup(anim_tiles->position, brush->position);
			vec2_dup(anim_tiles->half_size, brush->half_size);
			vec2_mul(anim_tiles->uv_scale, anim_tiles->half_size, (vec2){ 2.0, 2.0 });
			anim_tiles->animation = ANIMATION_WATER_TILE;
			anim_tiles->fps = 1.0;
			break;
		default:
			tiles = gfx_scene_new_obj(c->layer, SCENE_OBJECT_TILES);
			gfx_sprite_count_rows_cols(SPRITE_TERRAIN, &rows, &cols);
			vec2_dup(tiles->position, brush->position);
			vec2_dup(tiles->half_size, brush->half_size);
			vec2_mul(tiles->uv_scale, brush->half_size, (vec2){ 2.0, 2.0 });
			tiles->type = SPRITE_TERRAIN;
			tiles->sprite_x = (brush->tile - 1) % cols;
			tiles->sprite_y = (brush->tile - 1) / cols;
			break;
		}
	}
}

void
setup_layer(int layer, int w, int h, int *tiles)
{
	int *dup = malloc(sizeof(*tiles) * w * h);
	memcpy(dup, tiles, sizeof(*tiles) * w * h);

	for(int y = 0; y < h; y++)
	for(int x = 0; x < w; x++) {
		if(dup[x + y * w] > 0) {
			find_sprite(layer, x, y, w, h, dup);
		}
	}
	
	free(dup);
}

void
find_sprite(int layer, int x, int y, int w, int h, int *tiles)
{
	int x_end, y_end;
	int reference_tile = tiles[x + y * w];

	/* first find where x ends */
	for(x_end = x; x_end < w; x_end++) {
		if(tiles[x_end + y * w] != reference_tile) {
			break;
		}
	}

	/* now, find y where the size does not fit x_end */
	for(y_end = y + 1; y_end < h; y_end++) {
		for(int xx = x; xx < x_end; xx++) {
			if(tiles[xx + y_end * w] != reference_tile) {
				goto produce_rect;
			}
		}
	}

produce_rect:
	if(x_end - x == 0 || y_end - y == 0)
		return;

	for(int yy = y; yy < y_end; yy++)
	for(int xx = x; xx < x_end; xx++) {
		tiles[xx + yy * w] = 0;
	}
	
	create_sprite(layer, x, y, (x_end - x), (y_end - y), reference_tile - 1);
}

void
create_sprite(int layer, int x, int y, int w, int h, int tile)
{
	int rows, cols;
	if(tile != 4) {
		SceneTiles *sprite = gfx_scene_new_obj(layer, SCENE_OBJECT_TILES);

		gfx_sprite_count_rows_cols(SPRITE_TERRAIN, &rows, &cols);
		sprite->half_size[0] = (float)w * ENTITY_SCALE;
		sprite->half_size[1] = (float)h * ENTITY_SCALE;
		vec2_add_scaled(sprite->position, sprite->half_size, (vec2){ x, y }, 1.0);

		sprite->type = SPRITE_TERRAIN;
		sprite->sprite_x = tile % rows;
		sprite->sprite_y = tile / rows;
		sprite->uv_scale[0] = w;
		sprite->uv_scale[1] = h;
	} else {
		SceneAnimatedTiles *sprite = gfx_scene_new_obj(layer, SCENE_OBJECT_ANIMATED_TILES);

		gfx_sprite_count_rows_cols(SPRITE_TERRAIN, &rows, &cols);
		sprite->half_size[0] = (float)w * ENTITY_SCALE;
		sprite->half_size[1] = (float)h * ENTITY_SCALE;
		vec2_add_scaled(sprite->position, sprite->half_size, (vec2){ x, y }, 1.0);

		sprite->type = SPRITE_TERRAIN;
		sprite->sprite_x = tile % rows;
		sprite->sprite_y = tile / rows;
		sprite->uv_scale[0] = w;
		sprite->uv_scale[1] = h;
		sprite->animation = ANIMATION_WATER_TILE;
		sprite->fps = 1.0;
	}
}

void
map_insert_thing(Map *map, Thing *thing)
{
	if(map->things) {
		map_insert_thing_after(map, thing, map->things_end);
	} else {
		map->things = thing;
		map->things_end = thing;
	}
}

void
map_remove_thing(Map *map, Thing *thing)
{
	if(thing->next)
		thing->next->prev = thing->prev;
	if(thing->prev)
		thing->prev->next = thing->next;

	if(map->things_end == thing)
		map->things_end = thing->prev;

	if(map->things == thing)
		map->things = thing->next;
}

void
map_insert_thing_after(Map *map, Thing *thing, Thing *after)
{
	thing->next = after->next;
	if(after->next)
		after->next->prev = thing;
	else
	 	map->things_end = thing;

	thing->prev = after;
	after->next = thing;
}

void
map_thing_insert_brush(Thing *thing, MapBrush *brush)
{
	if(thing->brush_list) {
		map_thing_insert_brush_after(thing, brush, thing->brush_list_end);
	} else {
		thing->brush_list = brush;
		thing->brush_list_end = brush;
	}
}

void
map_thing_remove_brush(Thing *thing, MapBrush *brush)
{
	if(brush->prev)
		brush->prev->next = brush->next;
	if(brush->next)
		brush->next->prev = brush->prev;

	if(thing->brush_list == brush)
		thing->brush_list = brush->next;

	if(thing->brush_list_end == brush)
	 	thing->brush_list_end = brush->prev;
}

void
map_thing_insert_brush_after(Thing *thing, MapBrush *brush, MapBrush *after)
{
	brush->next = after->next;
	if(after->next)
		after->next->prev = brush;
	else
	 	thing->brush_list_end = brush;

	brush->prev = after;
	after->next = brush;
}

void
map_thing_insert_brush_before(Thing *thing, MapBrush *brush, MapBrush *before)
{
	brush->prev = before->prev;
	if(before->prev)
		before->prev->next = brush;
	else
	 	thing->brush_list = brush;
	
	brush->next = before;
	before->prev = brush;
}
