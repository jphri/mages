#include <stdbool.h>
#include <SDL.h>

#include "../graphics.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"
#include "../id.h"
#include "../global.h"

#define self ENT_DATA(ENTITY_DUMMY, self_id)
#define self_sprite gfx_scene_spr(self->sprite)

#define MOB_COMPONENT self->mob
#define BODY_COMPONENT (self->body)

#define self_body phx_data(BODY_COMPONENT.body)

#include "../entity_components.h"

EntityID 
ent_dummy_new(vec2 position)
{
	EntityID self_id = ent_new(ENTITY_DUMMY);
	process_init_components(self_id);

	self->sprite = gfx_scene_new_obj(0, SCENE_OBJECT_SPRITE);
	vec2_dup(self_body->position, position);
	vec2_dup(self_body->half_size, (vec2){ 1, 1 });
	vec2_dup(self_body->velocity, (vec2){ 0.0, 0.0 });
	self_body->is_static = false;
	self_body->collision_layer = PHX_LAYER_ENTITIES_BIT;
	self_body->collision_mask  = PHX_LAYER_ENTITIES_BIT | PHX_LAYER_MAP_BIT;
	self_body->user_data = make_gobj_id(GAME_OBJECT_TYPE_ENTITY, self_id);
	self_body->mass = 10.0;
	self_body->restitution = 0.0;
	self_body->damping = 5.0;

	self_sprite->type = SPRITE_ENTITIES;
	vec2_dup(self_sprite->sprite.position, position);
	vec2_dup(self_sprite->sprite.half_size, (vec2){ 1, 1 });
	vec4_dup(self_sprite->sprite.color, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	self_sprite->sprite.rotation = 0.0;
	self_sprite->sprite.sprite_id[0] = 0.0; 
	self_sprite->sprite.sprite_id[1] = 2.0;
	
	MOB_COMPONENT.health     = 10.0f;
	MOB_COMPONENT.health_max = 10.0f;

	return self_id;
}

void
ENTITY_DUMMY_update(EntityID self_id, float delta)
{
	(void)delta;
	vec2 delta_pos;

	(void)delta_pos;

	#define gb_player ENT_DATA(ENTITY_PLAYER, GLOBAL.player_id)
	#define gb_player_body phx_data(gb_player->body)

	//if(GLOBAL.player_id != 0) {
	//	vec2_sub(delta_pos, gb_player_body->position, self_body->position);
	//	vec2_normalize(delta_pos, delta_pos);
	//	vec2_mul(delta_pos, delta_pos, (vec2){ 15.0, 15.0 });
	//	vec2_dup(self_body->velocity, delta_pos);
	//	vec2_dup(self_sprite->sprite.position, self_body->position);
	//} else {
	//	vec2_dup(self_body->velocity, (vec2){ 0.0, 0.0 });
	//}

	//HitInfo *info = phx_hits(self->body);
	//for(; info; info = phx_hits_next(info)) {
	//	EntityID e_id;
	//	#define hit_object phx_data(info->id)
	//

	//	switch(id_type(hit_object->user_data)) {
	//	case ID_TYPE_ENTITY:
	//		e_id = id(hit_object->user_data);
	//		(void)e_id;
	//	default:
	//		do{}while(0);
	//	}
	//}

	vec2_dup(self_sprite->sprite.position, self_body->position);
	process_components(self_id);
}

void
ENTITY_DUMMY_render(EntityID id)
{
	(void)id;
}

void
ENTITY_DUMMY_del(EntityID self_id)
{
	gfx_scene_del_obj(self->sprite);
	process_del_components(self_id);
}
