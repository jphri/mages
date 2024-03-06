#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "../graphics.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"

#define SELF        ENT_DATA(ENTITY_PARTICLE, self_id)
#define SELF_SPRITE gfx_scene_spr(SELF->sprite)

#define BODY_COMPONENT (SELF->body)
#define SELF_BODY phx_data(BODY_COMPONENT.body)

#include "../entity_components.h"

static void particle_update(Entity *, float delta);
static void particle_die(Entity *);

static EntityInterface particle_int = {
	.update = particle_update,
	.die = particle_die
};

void     
ent_shot_particles(vec2 position, vec2 velocity, vec4 color, float time, int count)
{
	float va = sqrtf(vec2_dot(velocity, velocity));
	for(; count > 0; count--) {
		vec2 v, p;
		float r = (rand() / (float)RAND_MAX);
		float rr =  (rand() / (float)RAND_MAX);

		v[0] = cosf(r * 3.1415926535 * 2) * va * rr;
		v[1] = sinf(r * 3.1415926535 * 2) * va * rr;
		vec2_add_scaled(p, position, v, 0.01);

		ent_particle_new(p, v, color, time);
	}
}

Particle * 
ent_particle_new(vec2 position, vec2 velocity, vec4 color, float time)
{
	Particle *self = (Particle*)&ent_new(ENTITY_PARTICLE, &particle_int)->particle;

	self->body = phx_new();
	self->sprite = gfx_scene_new_obj(0, SCENE_OBJECT_SPRITE);
	self->time = time;

	vec2_dup(self->body->position, position);
	vec2_dup(self->body->velocity, velocity);
	self->body->half_size[0] = 0.1;
	self->body->half_size[1] = 0.1;
	self->body->is_static = false;
	self->body->solve_layer = 0;
	self->body->solve_mask  = PHX_LAYER_MAP_BIT;
	self->body->collision_layer = 0;
	self->body->collision_mask  = PHX_LAYER_MAP_BIT;
	self->body->mass = 0.0000001;
	self->body->restitution = 1.0;
	self->body->damping = 5.0;

	self->sprite->type = SPRITE_UI;
	vec2_dup(self->sprite->position, position);
	vec2_dup(self->sprite->half_size, (vec2){ 0.05, 0.05 });
	vec4_dup(self->sprite->color, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	self->sprite->rotation = 0.0;
	self->sprite->sprite_x = 0.0; self->sprite->sprite_y = 0.0;
	vec4_dup(self->sprite->color, color);
	
	return self;
}

void
particle_update(Entity *self_ent, float delta)
{
	Particle *self = (Particle*)self_ent;
	self->time -= delta;
	self->sprite->rotation += delta * 10.0;
	vec2_dup(self->sprite->position, VEC2_DUP(self->body->position));
	if(self->time < 0.0) {
		ent_del(self_ent);
	}
}

void
particle_die(Entity *self) 
{
	gfx_scene_del_obj(self->particle.sprite);
	phx_del(self->particle.body);
}
