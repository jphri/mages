#ifndef MAP_H
#define MAP_H

#include "defs.h"
#include "vecmath.h"

enum {
	THING_NULL,
	THING_WORLD_MAP,
	THING_PLAYER,
	THING_DUMMY,
	THING_DOOR,
	THING_BRUSH,
	LAST_THING
};

typedef struct CollisionData CollisionData;
typedef struct Thing Thing;
typedef struct MapBrush MapBrush;

struct CollisionData {
	vec2 position, half_size;
	CollisionData *next;
};

struct MapBrush{
	int tile;
	int collidable;
	vec2 position, half_size;
	MapBrush *next, *prev;
};


struct Thing {
	int type;
	int layer;
	vec2 position;
	float health, health_max;
	Direction direction;
	MapBrush *brush_list, *brush_list_end;
	Thing *next, *prev;
};

typedef struct {
	int w, h;
	int *tiles;
	CollisionData *collision;
	Thing *things, *things_end;
} Map;

Map *map_alloc(int w, int h);
Map *map_load(const char *file);
void map_free(Map *);

void map_insert_thing(Map *map, Thing *thing);
void map_remove_thing(Map *map, Thing *thing);
void map_insert_thing_after(Map *map, Thing *thing, Thing *after);
void map_insert_thing_before(Map *map, Thing *thing, Thing *before);

void map_thing_insert_brush(Thing *thing, MapBrush *brush);
void map_thing_remove_brush(Thing *thing, MapBrush *brush);
void map_thing_insert_brush_after(Thing *thing, MapBrush *brush, MapBrush *after);
void map_thing_insert_brush_before(Thing *thing, MapBrush *brush, MapBrush *before);

char *map_export(Map *map, size_t *out_data_size);
void map_set_gfx_scene(Map *map);
void map_set_phx_scene(Map *map);
void map_set_ent_scene(Map *map);

#endif
