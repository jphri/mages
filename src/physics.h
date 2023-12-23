#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdbool.h>

#include "vecmath.h"
#include "game_objects.h"

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
	BodyID body1, body2;
	vec2 normal, pierce;
} Contact;

typedef struct {
	BodyID body1, body2;
	vec2 normal, pierce;
} SolveInfo;

typedef struct {
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

	unsigned int user_data;
	bool is_static, no_update;
} Body;

void phx_init(void);
void phx_end(void);
void phx_reset(void);

void phx_set_grid_size(int w, int h);
void phx_set_pre_solve(void (*)(Contact *contact));

BodyID phx_new(void);
void   phx_del(BodyID self);
void   phx_update(float delta);
void   phx_draw(void);
Body  *phx_data(BodyID self);

GameObjectRegistry phx_object_descr(void);

#endif
