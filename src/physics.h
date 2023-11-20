#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdbool.h>

#include "vecmath.h"
#include "id.h"

#define PHX_LAYERS \
	PHX_LAYER(MAP) \
	PHX_LAYER(ENTITIES)

typedef enum {
	#define PHX_LAYER(LAYER_NAME) \
		PHX_LAYER_##LAYER_NAME,
	PHX_LAYERS
	#undef PHX_LAYER
} PhxLayers;

enum {
	#define PHX_LAYER(LAYER_NAME) \
		PHX_LAYER_##LAYER_NAME##_BIT = (1 << PHX_LAYER_##LAYER_NAME),
	PHX_LAYERS
	#undef PHX_LAYER
};

typedef struct {
	vec2 position;
	vec2 half_size;
	vec2 velocity;

	unsigned int solve_layer;
	unsigned long long int solve_mask;
	unsigned int collision_layer;
	unsigned long long int collision_mask;

	unsigned int user_data;
	bool is_static, no_update;
} Body;

typedef struct {
	BodyID id;
	vec2 normal;
	vec2 pierce;
} HitInfo;

void phx_init();
void phx_end();
void phx_reset();

BodyID phx_new();
void   phx_del(BodyID self);
void   phx_update(float delta);
void   phx_draw();
Body  *phx_data(BodyID self);

HitInfo *phx_hits(BodyID self_id, unsigned int *count);

#endif
