#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "../graphics.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"

#define SELF      ENT_DATA(ENTITY_PARTICLE, self_id)
#define SELF_SPRITE gfx_scene_spr(SELF->sprite)

#define BODY_COMPONENT (SELF->body)
#define SELF_BODY phx_data(BODY_COMPONENT.body)

#include "../entity_components.h"

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

EntityID 
ent_particle_new(vec2 position, vec2 velocity, vec4 color, float time)
{
	EntityID self_id = ent_new(ENTITY_PARTICLE);

	process_init_components(self_id);
	
	SELF->sprite = gfx_scene_new_obj(0, SCENE_OBJECT_SPRITE);
	SELF->time = time;

	vec2_dup(SELF_BODY->position, position);
	vec2_dup(SELF_BODY->velocity, velocity);
	SELF_BODY->half_size[0] = 0.1;
	SELF_BODY->half_size[1] = 0.1;
	SELF_BODY->is_static = false;
	SELF_BODY->solve_layer = 0;
	SELF_BODY->solve_mask  = 0;
	SELF_BODY->collision_layer = PHX_LAYER_ENTITIES_BIT;
	SELF_BODY->collision_mask  = PHX_LAYER_MAP_BIT | PHX_LAYER_ENTITIES_BIT;
	SELF_BODY->user_data = self_id;
	SELF_BODY->mass = 0.0000001;
	SELF_BODY->restitution = 1.0;
	SELF_BODY->damping = 5.0;

	SELF_SPRITE->type = SPRITE_ENTITIES;
	vec2_dup(SELF_SPRITE->sprite.position, position);
	vec2_dup(SELF_SPRITE->sprite.half_size, (vec2){ 0.1, 0.1 });
	vec4_dup(SELF_SPRITE->sprite.color, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	SELF_SPRITE->sprite.rotation = 0.0;
	SELF_SPRITE->sprite.sprite_id[0] = 0.0; SELF_SPRITE->sprite.sprite_id[1] = 1.0;
	vec4_dup(SELF_SPRITE->sprite.color, color);
	
	return self_id;
}

void
ENTITY_PARTICLE_update(EntityID self_id, float delta)
{
	SELF->time -= delta;
	vec2_dup(SELF_SPRITE->sprite.position, SELF_BODY->position);
	if(SELF->time < 0.0) {
		ent_del(self_id);
	}
}

void 
ENTITY_PARTICLE_render(EntityID id) {(void)id;} 

void
ENTITY_PARTICLE_del(EntityID self_id) 
{
	gfx_scene_del_obj(SELF->sprite);
	process_del_components(self_id);
}
