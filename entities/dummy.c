#include <stdbool.h>
#include <SDL2/SDL.h>

#include "../graphics.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"
#include "../id.h"

EntityID 
ent_dummy_new(vec2 position)
{
	EntityID self_id = ent_new(ENTITY_DUMMY);

	#define self ENT_DATA(ENTITY_DUMMY, self_id)
	#define self_body phx_data(self->body)
	#define self_sprite gfx_scene_spr_data(self->sprite)

	self->body = phx_new();
	self->sprite = gfx_scene_new_spr(0);
	vec2_dup(self_body->position, position);
	vec2_dup(self_body->half_size, (vec2){ 1, 1 });
	vec2_dup(self_body->velocity, (vec2){ 0.0, 0.0 });
	self_body->is_static = false;
	self_body->solve_layer     = 0x00;
	self_body->solve_mask      = 0x02;
	self_body->collision_layer = 0x00;
	self_body->collision_mask  = 0x03;
	self_body->user_data = make_id_descr(ID_TYPE_ENTITY, self_id);

	self_sprite->type = SPRITE_ENTITIES;
	vec2_dup(self_sprite->sprite.position, position);
	vec2_dup(self_sprite->sprite.half_size, (vec2){ 1, 1 });
	vec4_dup(self_sprite->sprite.color, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	self_sprite->sprite.rotation = 0.0;
	self_sprite->sprite.sprite_id[0] = 0.0; 
	self_sprite->sprite.sprite_id[1] = 2.0;
	self->health = 10.0f;

	return self_id;

	#undef self_id
	#undef self_body
	#undef self_sprite
}

void
ENTITY_DUMMY_update(EntityID id, float delta)
{
	(void)id;
	(void)delta;
}

void
ENTITY_DUMMY_render(EntityID id)
{
	(void)id;
}

void
ENTITY_DUMMY_del(EntityID self_id)
{
	#define self ENT_DATA(ENTITY_DUMMY, self_id)
	phx_del(self->body);
	gfx_scene_del_spr(self->sprite);
}
