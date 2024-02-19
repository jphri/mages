#ifndef MAP_H
#define MAP_H

#include "vecmath.h"
typedef struct CollisionData CollisionData;
typedef struct MarkData MarkData;

struct CollisionData {
	vec2 position, half_size;
	CollisionData *next;
};

struct MarkData {
	const char *name;
	Rectangle rect;
	MarkData *next;
};

typedef struct {
	int w, h;
	int *tiles;
	CollisionData *collision;
	MarkData *mark;
} Map;

Map *map_alloc(int w, int h);
Map *map_load(const char *file);
void map_free(Map *);

MarkData *map_find_mark(Map *, const char *name);

char *map_export(Map *map, size_t *out_data_size);
void map_set_gfx_scene(Map *map);
void map_set_phx_scene(Map *map);

#endif
