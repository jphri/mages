#include <stdbool.h>
#include <SDL.h>
#include "vecmath.h"

#include "../graphics.h"
#include "../physics.h"
#include "../entity.h"
#include "../id.h"
#include "../global.h"

#include "../entity_components.h"

static void dummy_update(Entity *self_id, float delta);
static void dummy_take_damage(Entity *self_id, float damage);
static void dummy_die(Entity *self_id);

static EntityInterface dummy_in = {
	.update = dummy_update, 
	.take_damage = dummy_take_damage,
	.die = dummy_die
};

Dummy * 
ent_dummy_new(vec2 position)
{
	Entity *entity = ent_new(ENTITY_DUMMY, &dummy_in);
	Dummy *self = &entity->dummy;

	self->sprite = gfx_scene_new_obj(1, SCENE_OBJECT_SPRITE);
	self->body = phx_new();

	vec2_dup(self->body->position, position);
	vec2_dup(self->body->half_size, (vec2){ 1 * ENTITY_SCALE, 1 * ENTITY_SCALE });
	vec2_dup(self->body->velocity, (vec2){ 0.0, 0.0 });
	self->body->is_static = false;
	self->body->collision_layer = PHX_LAYER_ENTITIES_BIT;
	self->body->collision_mask  = PHX_LAYER_ENTITIES_BIT | PHX_LAYER_MAP_BIT;
	self->body->entity = entity;
	self->body->mass = 10.0;
	self->body->restitution = 0.0;
	self->body->damping = 5.0;

	self->sprite->type = SPRITE_ENTITIES;
	vec2_dup(self->sprite->position, position);
	vec2_dup(self->sprite->half_size, (vec2){ 1 * ENTITY_SCALE, 1 * ENTITY_SCALE });
	vec4_dup(self->sprite->color, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	self->sprite->rotation = 0.0;
	self->sprite->sprite_x = 0; 
	self->sprite->sprite_y = 2;
	self->sprite->uv_scale[0] = 1.0;
	self->sprite->uv_scale[1] = 1.0;
	
	self->mob.health     = 10.0f;
	self->mob.health_max = 10.0f;

	return self;
}

void
dummy_update(Entity *self, float delta)
{
	(void)delta;
	vec2 delta_pos;

	(void)delta_pos;

	vec2_dup(self->dummy.sprite->position, self->dummy.body->position);
	mob_process(self, &self->dummy.mob);
}

void
dummy_die(Entity *self)
{
	gfx_scene_del_obj(self->dummy.sprite);
	phx_del(self->dummy.body);
}

void
dummy_take_damage(Entity *self, float damage)
{
	self->dummy.mob.health += damage;
}
