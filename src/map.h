#ifndef MAP_H
#define MAP_H

#include "defs.h"
#include "vecmath.h"

enum {
	THING_NULL,
	THING_PLAYER,
	THING_DUMMY,
	LAST_THING
};

typedef struct CollisionData CollisionData;
typedef struct Thing Thing;

struct CollisionData {
	vec2 position, half_size;
	CollisionData *next;
};

struct Thing {
	int type;
	vec2 position;
	float health, health_max;
	Direction direction;

	Thing *next;
	Thing *prev;
};

typedef struct {
	int w, h;
	int *tiles;
	CollisionData *collision;
	Thing *things;
} Map;

Map *map_alloc(int w, int h);
Map *map_load(const char *file);
void map_free(Map *);

char *map_export(Map *map, size_t *out_data_size);
void map_set_gfx_scene(Map *map);
void map_set_phx_scene(Map *map);
void map_set_ent_scene(Map *map);

#endif
