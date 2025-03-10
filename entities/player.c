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

	self->body = phx_new();
	vec2_dup(self_body->position, position);
	vec2_dup(self_body->half_size, (vec2){ 15, 15 });
	vec2_dup(self_body->velocity, (vec2){ 0.0, 0.0 });
	self_body->is_static = false;
	self_body->solve_layer     = 0x00;
	self_body->solve_mask      = 0x01;
	self_body->collision_layer = 0x00;
	self_body->collision_mask  = 0x01;

	return self_id;
	#undef self_id
	#undef self_body
}

void
ent_player_update(EntityID self_id, float delta) 
{
	#define self ((EntityPlayer*)ent_data(self_id))
	#define self_body phx_data(self->body)

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

	#undef self_id
	#undef self_body
}

void
ent_player_render(EntityID self_id)
{
	#define self ((EntityPlayer*)ent_data(self_id))
	#define self_body phx_data(self->body)
	
	gfx_draw_sprite(&(Sprite) {
		.position  = { self_body->position[0], self_body->position[1] },
		.color     = { 1.0, 1.0, 1.0, 1.0 },
		.half_size = { self_body->half_size[0], self_body->half_size[1] },
		.sprite_id = { 0.0, 0.0 },
		.sprite_type = SPRITE_PLAYER
	});
}

void
ent_player_del(EntityID self_id) 
{
	#define self ((EntityPlayer*)ent_data(self_id))
	phx_del(self->body);
}

