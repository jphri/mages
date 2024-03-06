#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#include "../graphics.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"
#include "../id.h"
#include "../audio.h"

#define SELF      ENT_DATA(ENTITY_FIREBALL, self_id)
#define SELF_SPRITE gfx_scene_spr(SELF->sprite)

#define BODY_COMPONENT (SELF->body)
#define SELF_BODY phx_data(BODY_COMPONENT.body)

#include "../entity_components.h"

static void collision_callback(BodyID self_body, BodyID other, Contact *contact);
static void fireball_update(EntityID self_id, float delta);
static void fireball_die(EntityID self_id);

static EntityInterface fireball_interface = {
	.update = fireball_update,
	.die = fireball_die
};

EntityID
ent_fireball_new(EntityID caster, vec2 position, vec2 vel)
{
	EntityID self_id = ent_new(ENTITY_FIREBALL, &fireball_interface);

	process_init_components(self_id);

	SELF->sprite = gfx_scene_new_obj(0, SCENE_OBJECT_SPRITE);
	SELF->caster = caster;
	SELF->time = 0;

	vec2_dup(SELF_BODY->position, position);
	vec2_dup(SELF_BODY->velocity, vel);
	SELF_BODY->half_size[0] = 0.5 * ENTITY_SCALE;
	SELF_BODY->half_size[1] = 0.5 * ENTITY_SCALE;
	SELF_BODY->is_static = false;
	SELF_BODY->solve_layer = 0;
	SELF_BODY->solve_mask  = 0;
	SELF_BODY->collision_layer = 0;
	SELF_BODY->collision_mask  = PHX_LAYER_MAP_BIT | PHX_LAYER_ENTITIES_BIT;
	SELF_BODY->pre_solve = collision_callback;
	SELF_BODY->entity = self_id;
	SELF_BODY->mass = 1.0;
	SELF_BODY->restitution = 0.55;
	SELF_BODY->damping = 1.0;

	SELF_SPRITE->type = SPRITE_ENTITIES;
	vec2_dup(SELF_SPRITE->position, position);
	vec2_dup(SELF_SPRITE->half_size, (vec2){ 0.5 * ENTITY_SCALE, 0.5 * ENTITY_SCALE });
	vec4_dup(SELF_SPRITE->color, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	SELF_SPRITE->rotation = 0.0;
	SELF_SPRITE->sprite_x = 0; SELF_SPRITE->sprite_y = 1;
	SELF_SPRITE->rotation = atan2f(-vel[1], vel[0]);

	SELF->damage.damage = -1.0;
	return self_id;
}

void
fireball_update(EntityID self_id, float delta)
{
	SELF->time += delta;
	vec2_dup(SELF_SPRITE->position, SELF_BODY->position);

	if(SELF->time > 1.0) {
		ent_del(self_id);
		ent_shot_particles(VEC2_DUP(SELF_BODY->position), VEC2_DUP(SELF_BODY->velocity), (vec4){ 1.0, 1.0, 0.0, 1.0 }, 0.5, 10);
	}
}

void
fireball_die(EntityID self_id)
{
	gfx_scene_del_obj(SELF->sprite);
	process_del_components(self_id);
}

void
collision_callback(BodyID self_body, BodyID other, Contact *contact)
{
	EntityID self_id, other_ent;

	self_id = phx_data(self_body)->entity;
	if(!phx_data(other)->entity) {
		ent_shot_particles(VEC2_DUP(SELF_BODY->position), VEC2_DUP(SELF_BODY->velocity), (vec4){ 1.0, 1.0, 0.0, 1.0 }, 0.25, 4);
		audio_sfx_play(AUDIO_MIXER_SFX, AUDIO_BUFFER_FIREBALL_HIT, 1.0);
		ent_del(self_id);
		return;
	}
	other_ent = phx_data(other)->entity;

	if(other_ent == SELF->caster) {
		contact->active = false;
		return;
	}

	if(!ENT_IMPLEMENTS(other_ent, take_damage))
		return;
	
	ent_take_damage(other_ent, SELF->damage.damage, VEC2_DUP(SELF_BODY->position));

	ent_shot_particles(VEC2_DUP(SELF_BODY->position), VEC2_DUP(SELF_BODY->velocity), (vec4){ 1.0, 1.0, 0.0, 1.0 }, 0.25, 4);
	audio_sfx_play(AUDIO_MIXER_SFX, AUDIO_BUFFER_FIREBALL_HIT, 1.0);
	ent_del(self_id);
}

