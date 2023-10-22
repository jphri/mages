#include <stdbool.h>
#include <SDL2/SDL.h>

#include "../graphics.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"

EntityID
ent_player_new(vec2 position)
{
	EntityID self_id = ent_new(ENTITY_PLAYER);
	#define self ((EntityPlayer*)ent_data(self_id))
	#define self_body phx_data(self->body)
	#define self_sprite gfx_scene_spr_data(self->sprite)

	self->body = phx_new();
	self->sprite = gfx_scene_new_spr(0);
	vec2_dup(self_body->position, position);
	vec2_dup(self_body->half_size, (vec2){ 15, 15 });
	vec2_dup(self_body->velocity, (vec2){ 0.0, 0.0 });
	self_body->is_static = false;
	self_body->solve_layer     = 0x00;
	self_body->solve_mask      = 0x01;
	self_body->collision_layer = 0x00;
	self_body->collision_mask  = 0x01;

	self_sprite->sprite_type = SPRITE_PLAYER;
	vec2_dup(self_sprite->position, position);
	vec2_dup(self_sprite->half_size, (vec2){ 15, 15 });
	vec4_dup(self_sprite->color, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	self_sprite->rotation = 0.0;
	self_sprite->sprite_id[0] = 0.0; self_sprite->sprite_id[1] = 0.0;

	return self_id;

	#undef self_id
	#undef self_body
	#undef self_sprite
}

void
ent_player_update(EntityID self_id, float delta) 
{
	#define self ((EntityPlayer*)ent_data(self_id))
	#define self_body phx_data(self->body)
	#define self_sprite gfx_scene_spr_data(self->sprite)

	(void)delta;

	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	unsigned int hit_count;

	vec2_dup(self_body->velocity, (vec2){ 0.0, 0.0 });
	if(keys[SDL_SCANCODE_W])
		self_body->velocity[1] -= 100;
	if(keys[SDL_SCANCODE_S])
		self_body->velocity[1] += 100;
	if(keys[SDL_SCANCODE_A])
		self_body->velocity[0] -= 100;
	if(keys[SDL_SCANCODE_D])
		self_body->velocity[0] += 100;

	if(keys[SDL_SCANCODE_K])
		ent_del(self_id);

	HitInfo *info = phx_hits(self->body, &hit_count);
	for(unsigned int i = 0; i < hit_count; i++)
		printf("Hit ID: %d\n", info->id);

	vec2_dup(self_sprite->position, self_body->position);

	#undef self_id
	#undef self_body
	#undef self_sprite
}

void
ent_player_render(EntityID self_id)
{
	#define self ((EntityPlayer*)ent_data(self_id))
	#define self_body phx_data(self->body)
	(void)self_id;

	#undef self
	#undef self_body
}

void
ent_player_del(EntityID self_id) 
{
	#define self ((EntityPlayer*)ent_data(self_id))
	phx_del(self->body);
	gfx_scene_del_spr(self->sprite);
}

