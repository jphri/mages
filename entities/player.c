#include <stdbool.h>
#include <SDL2/SDL.h>

#include "../graphics.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"
#include "../id.h"

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
	self_sprite->sprite.sprite_id[0] = 0.0; self_sprite->sprite.sprite_id[1] = 0.0;
	self->fired = 0;

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
	int mouse_x, mouse_y;

	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	unsigned int hit_count;
	int state = SDL_GetMouseState(&mouse_x, &mouse_y);


	vec2_dup(self_body->velocity, (vec2){ 0.0, 0.0 });
	if(keys[SDL_SCANCODE_W])
		self_body->velocity[1] -= 10;
	if(keys[SDL_SCANCODE_S])
		self_body->velocity[1] += 10;
	if(keys[SDL_SCANCODE_A])
		self_body->velocity[0] -= 10;
	if(keys[SDL_SCANCODE_D])
		self_body->velocity[0] += 10;

	if(keys[SDL_SCANCODE_K])
		ent_del(self_id);

	if(SDL_BUTTON(SDL_BUTTON_LEFT) & state) {
		if(!self->fired) {
			vec2 mouse_pos;
			mouse_pos[0] = mouse_x, mouse_pos[1] = mouse_y;

			gfx_pixel_to_world(mouse_pos, mouse_pos);
			vec2_sub(mouse_pos, mouse_pos, self_body->position);
			vec2_normalize(mouse_pos, mouse_pos);
			vec2_mul(mouse_pos, mouse_pos, (vec2){ 10.0, 10.0 });
			ent_fireball_new(self_id, self_body->position, mouse_pos);
			self->fired = 1;
		}
	} else {
		self->fired = 0;
	}

	HitInfo *info = phx_hits(self->body, &hit_count);
	for(unsigned int i = 0; i < hit_count; i++) {
		EntityID e_id;
		#define hit_object phx_data(info[i].id)

		switch(id_type(hit_object->user_data)) {
		case ID_TYPE_ENTITY:
			e_id = id(hit_object->user_data);
			if(ent_type(e_id) == ENTITY_DUMMY) {
				ent_del(e_id);
			}
		default:
			do{}while(0);
		}
	}
	vec2_dup(self_sprite->sprite.position, self_body->position);

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
