#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include "vecmath.h"

#include "../graphics.h"
#include "physics.h"
#include "../entity.h"
#include "../id.h"
#include "../audio.h"

static void collision_callback(Body * self_body, Body * other, Contact *contact);
static void fireball_update(Entity * self_id, float delta);
static void fireball_die(Entity * self_id);

static EntityInterface fireball_interface = {
	.update = fireball_update,
	.die = fireball_die
};

Fireball *
ent_fireball_new(Entity* caster, vec2 position, vec2 vel)
{
	Fireball *self = &ent_new(ENTITY_FIREBALL, &fireball_interface)->fireball;

	self->body = phx_new();
	self->sprite = gfx_scene_new_obj(1, SCENE_OBJECT_SPRITE);
	self->caster = caster;
	self->time = 0;

	vec2_dup(self->body->position, position);
	vec2_dup(self->body->velocity, vel);
	self->body->half_size[0] = 0.5 * ENTITY_SCALE;
	self->body->half_size[1] = 0.5 * ENTITY_SCALE;
	self->body->is_static = false;
	self->body->solve_layer = 0;
	self->body->solve_mask  = 0;
	self->body->collision_layer = 0;
	self->body->collision_mask  = PHX_LAYER_MAP_BIT | PHX_LAYER_ENTITIES_BIT;
	self->body->pre_solve = collision_callback;
	self->body->entity = (Entity*)self;;
	self->body->mass = 1.0;
	self->body->restitution = 0.55;
	self->body->damping = 1.0;

	self->sprite->type = SPRITE_ENTITIES;
	vec2_dup(self->sprite->position, position);
	vec2_dup(self->sprite->half_size, (vec2){ 0.5 * ENTITY_SCALE, 0.5 * ENTITY_SCALE });
	vec4_dup(self->sprite->color, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	self->sprite->rotation = 0.0;
	self->sprite->sprite_x = 0; self->sprite->sprite_y = 1;
	self->sprite->rotation = atan2f(-vel[1], vel[0]);
	self->sprite->uv_scale[0] = 1.0;
	self->sprite->uv_scale[1] = 1.0;

	self->damage = -1.0;
	return self;
}

void
fireball_update(Entity * ent, float delta)
{
	Fireball *self = &ent->fireball;

	self->time += delta;
	vec2_dup(self->sprite->position, self->body->position);

	if(self->time > 1.0) {
		ent_del(ent);
		ent_shot_particles(VEC2_DUP(self->body->position), VEC2_DUP(self->body->velocity), (vec4){ 1.0, 1.0, 0.0, 1.0 }, 0.5, 10);
	}
}

void
fireball_die(Entity *self)
{
	gfx_scene_del_obj(self->fireball.sprite);
	phx_del(self->fireball.body);
}

void
collision_callback(Body *self_body, Body *other, Contact *contact)
{
	Entity *self, *other_ent;

	self = self_body->entity;
	if(!other->entity) {
		ent_shot_particles(VEC2_DUP(self->fireball.body->position), VEC2_DUP(self->fireball.body->velocity), (vec4){ 1.0, 1.0, 0.0, 1.0 }, 0.25, 4);
		audio_sfx_play(AUDIO_MIXER_SFX, AUDIO_BUFFER_FIREBALL_HIT, 1.0);
		ent_del(self);
		return;
	}
	other_ent = other->entity;

	if(other_ent == self->fireball.caster) {
		contact->active = false;
		return;
	}

	if(ENT_IMPLEMENTS(other_ent, take_damage)) {
		ent_take_damage(other_ent, self->fireball.damage, VEC2_DUP(self->fireball.body->position));
	}

	ent_shot_particles(VEC2_DUP(self->fireball.body->position), VEC2_DUP(self->fireball.body->velocity), (vec4){ 1.0, 1.0, 0.0, 1.0 }, 0.25, 4);
	audio_sfx_play(AUDIO_MIXER_SFX, AUDIO_BUFFER_FIREBALL_HIT, 1.0);
	ent_del(self);
}

