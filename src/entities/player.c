#include <stdbool.h>
#include <SDL2/SDL.h>

#include "../global.h"
#include "../graphics.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"
#include "../id.h"

#define self ENT_DATA(ENTITY_PLAYER, self_id)
#define self_body phx_data(self->body)
#define self_sprite gfx_scene_animspr(self->sprite)

#define MOB_COMPONENT self->mob

#include "../entity_components.h"

EntityID
ent_player_new(vec2 position)
{
	EntityID self_id = ent_new(ENTITY_PLAYER);
	
	self->body = phx_new();
	self->sprite = gfx_scene_new_obj(0, SCENE_OBJECT_ANIMATED_SPRITE);
	vec2_dup(self_body->position, position);
	vec2_dup(self_body->half_size, (vec2){ 1, 1 });
	vec2_dup(self_body->velocity, (vec2){ 0.0, 0.0 });
	self_body->is_static = false;
	self_body->solve_layer     = 0x00;
	self_body->solve_mask      = 0x02;
	self_body->collision_layer = 0x00;
	self_body->collision_mask  = 0x03;
	self_body->user_data = make_id_descr(ID_TYPE_ENTITY, self_id);

	self_sprite->sprite.type = SPRITE_ENTITIES;
	vec2_dup(self_sprite->sprite.position, position);
	vec2_dup(self_sprite->sprite.half_size, (vec2){ 1, 1 });
	vec4_dup(self_sprite->sprite.color, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	self_sprite->sprite.rotation = 0.0;
	self_sprite->sprite.sprite_id[0] = 0.0; self_sprite->sprite.sprite_id[1] = 0.0;
	self_sprite->fps = 0.0;
	self_sprite->time = 0.0;
	self_sprite->sprite_id[0] = 0.0, self_sprite->sprite_id[1] = 0.0;
	self_sprite->animation = ANIMATION_NULL;
	self->fired = 0;

	MOB_COMPONENT.health = 10;
	MOB_COMPONENT.health_max = 10;

	return self_id;
}

void
ENTITY_PLAYER_update(EntityID self_id, float delta) 
{
	(void)delta;
	int mouse_x, mouse_y;

	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	int state = SDL_GetMouseState(&mouse_x, &mouse_y);

	vec2_dup(self_body->velocity, (vec2){ 0.0, 0.0 });
	if(keys[SDL_SCANCODE_W])
		self_body->velocity[1] -= 10;
	if(keys[SDL_SCANCODE_S])
		self_body->velocity[1] += 10;
	if(keys[SDL_SCANCODE_A]) {
		self_sprite->sprite.half_size[0] = -1.0;
		self_body->velocity[0] -= 10;
	}
	if(keys[SDL_SCANCODE_D]) {
		self_sprite->sprite.half_size[0] = 1.0;
		self_body->velocity[0] += 10;
	}

	if(self_body->velocity[0] != 0 || self_body->velocity[1] != 0) {
		if(!self->moving) {
			self_sprite->animation = ANIMATION_PLAYER_MOVEMENT;
			self_sprite->fps = 5.0;
			self_sprite->time = 0.0;
			self->moving = true;
		}
	} else {
		self_sprite->animation = ANIMATION_NULL;
		self_sprite->fps = 0.0;
		self_sprite->time = 0.0;
		self->moving = false;
	}

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

	vec2_dup(self_sprite->sprite.position, self_body->position);
	process_components(self_id);
}

void
ENTITY_PLAYER_render(EntityID self_id)
{
	(void)self_id;
}

void
ENTITY_PLAYER_del(EntityID self_id) 
{
	phx_del(self->body);
	gfx_scene_del_obj(self->sprite);

	GLOBAL.player_id = 0;
}
