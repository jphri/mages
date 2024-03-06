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

void
collision_callback(BodyID self_body, BodyID other, Contact *contact)
{
	EntityID self_id, other_ent;
	EntityMob *mob;

	if(!phx_data(other)->entity) {
		return;
	}

	self_id = phx_data(self_body)->entity;
	other_ent = phx_data(other)->entity;

	if(other_ent == SELF->caster) {
		contact->active = false;
		return;
	}

	if(ent_type(other_ent) == ENTITY_PARTICLE) {
		return;
	}

	mob = ent_component(other_ent, ENTITY_COMP_MOB);
	if(mob) {
		mob->health += SELF->damage.damage;
		ent_damage_number(VEC2_DUP(SELF_BODY->position), SELF->damage.damage);
	}

	ent_shot_particles(VEC2_DUP(SELF_BODY->position), VEC2_DUP(SELF_BODY->velocity), (vec4){ 1.0, 1.0, 0.0, 1.0 }, 0.25, 4);
	audio_sfx_play(AUDIO_MIXER_SFX, AUDIO_BUFFER_FIREBALL_HIT, 1.0);
	ent_del(self_id);
}

EntityID
ent_fireball_new(EntityID caster, vec2 position, vec2 vel)
{
	EntityID self_id = ent_new(ENTITY_FIREBALL);

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
ENTITY_FIREBALL_update(EntityID self_id, float delta)
{

	SELF->time += delta;
	vec2_dup(SELF_SPRITE->position, SELF_BODY->position);

	//HitInfo *hits = phx_hits(SELF->body);
	//for(; hits; hits = phx_hits_next(hits)) {
	//	BodyID who = hits->id;
	//	EntityID ent_id;
	//	printf("%p\n", (void*)hits);

	//	EntityMob *mob;

	//	d++;

	//	switch(id_type(phx_data(who)->user_data)) {
	//	case ID_TYPE_ENTITY:
	//		ent_id = id(phx_data(who)->user_data);
	//		if(ent_type(ent_id) == ENTITY_PLAYER)
	//			break;
	//		mob = ent_component(ent_id, ENTITY_COMP_MOB);
	//		if(mob) {
	//			ent_del(self_id);
	//			mob->health += SELF->damage.damage;
	//			ent_damage_number(SELF_BODY->position, SELF->damage.damage);
	//		}
	//		break;
	//	case ID_TYPE_NULL:
	//		ent_del(self_id);
	//		break;

	//	default:
	//		do {} while(0);
	//	}
	//}

	if(SELF->time > 1.0) {
		ent_del(self_id);
		ent_shot_particles(VEC2_DUP(SELF_BODY->position), VEC2_DUP(SELF_BODY->velocity), (vec4){ 1.0, 1.0, 0.0, 1.0 }, 0.5, 10);
	}
}

void
ENTITY_FIREBALL_render(EntityID id) {(void)id;}

void
ENTITY_FIREBALL_del(EntityID self_id)
{
	gfx_scene_del_obj(SELF->sprite);
	process_del_components(self_id);
}
