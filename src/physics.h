#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdbool.h>

#include "vecmath.h"
#include "game_objects.h"
#include "defs.h"

#define GRID_TILE_SIZE 16

#define PHX_LAYERS \
	PHX_LAYER(MAP) \
	PHX_LAYER(ENTITIES) \
	PHX_LAYER(PROJECTILE) 

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
	bool active;
	Body *body1, *body2;
	vec2 normal, pierce;
} Contact;

struct Body {
	vec2 accel;
	vec2 position;
	vec2 half_size;
	vec2 velocity;

	float mass, restitution;
	float damping;

	unsigned int solve_layer;
	unsigned long long int solve_mask;
	unsigned int collision_layer;
	unsigned long long int collision_mask;

	void (*pre_solve)(Body *self, Body *other, Contact *contact);
	bool is_static, no_update;

	Entity *entity;
};

typedef struct {
	Body *body1, *body2;
	vec2 normal, pierce;
} SolveInfo;

void phx_init(void);
void phx_end(void);
void phx_reset(void);

void phx_set_grid_size(int w, int h);

Body *phx_new(void);
void  phx_del(Body *body);
void  phx_update(float delta);

#endif
