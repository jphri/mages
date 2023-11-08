#include <stdbool.h>
#include <stdio.h>

#include "../graphics.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"
#include "../id.h"

#define SELF ((EntityFireball*)ent_data(self))
#define SELF_BODY phx_data(SELF->body)
#define SELF_SPRITE gfx_scene_spr_data(SELF->sprite)

EntityID 
ent_fireball_new(EntityID caster, vec2 position, vec2 vel)
{
	EntityID self = ent_new(ENTITY_FIREBALL);
	
	SELF->body = phx_new();
	SELF->sprite = gfx_scene_new_spr(0);
	SELF->caster = caster;
	SELF->time = 0;

	vec2_dup(SELF_BODY->position, position);
	vec2_dup(SELF_BODY->velocity, vel);
	SELF_BODY->half_size[0] = 0.5;
	SELF_BODY->half_size[1] = 0.5;
	SELF_BODY->is_static = false;
	SELF_BODY->solve_layer = 0x00;
	SELF_BODY->solve_mask  = 0x00;
	SELF_BODY->collision_layer = 0x00;
	SELF_BODY->collision_mask  = 0x03;
	SELF_BODY->user_data = make_id_descr(ID_TYPE_ENTITY, self);

	SELF_SPRITE->type = SPRITE_ENTITIES;
	vec2_dup(SELF_SPRITE->sprite.position, position);
	vec2_dup(SELF_SPRITE->sprite.half_size, (vec2){ 0.5, 0.5 });
	vec4_dup(SELF_SPRITE->sprite.color, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	SELF_SPRITE->sprite.rotation = 0.0;
	SELF_SPRITE->sprite.sprite_id[0] = 0.0; SELF_SPRITE->sprite.sprite_id[1] = 1.0;
	SELF_SPRITE->sprite.rotation = atan2f(-vel[1], vel[0]);

	return self;
}

void
ent_fireball_update(EntityID self, float delta)
{
	unsigned int count;

	SELF->time += delta;
	vec2_dup(SELF_SPRITE->sprite.position, SELF_BODY->position);

	HitInfo *hits = phx_hits(SELF->body, &count);
	for(unsigned int i = 0; i < count; i++) {
		BodyID who = hits[i].id;
		EntityID ent_id;

		switch(id_type(phx_data(who)->user_data)) {
		case ID_TYPE_ENTITY:
			ent_id = id(phx_data(who)->user_data);
			if(ent_type(ent_id) == ENTITY_DUMMY) {
				ent_del(ent_id);
				ent_del(self);
			}
			break;
		case ID_TYPE_NULL:
			ent_del(self);
			break;
		default:
			do {} while(0);
		}
	}

	if(SELF->time > 1.0)
		ent_del(self);
}

void
ent_fireball_del(EntityID self) 
{
	phx_del(SELF->body);
	gfx_scene_del_spr(SELF->sprite);
}
