#include <stdbool.h>
#include <SDL.h>

#include "../global.h"
#include "../graphics.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"
#include "../id.h"
#include "../audio.h"
#include "../events.h"

#define self ENT_DATA(ENTITY_PLAYER, self_id)
#define self_sprite gfx_scene_animspr(self->sprite)

#define MOB_COMPONENT self->mob
#define BODY_COMPONENT self->body

#define self_body phx_data(BODY_COMPONENT.body)

#define SPEED 50

#include "../entity_components.h"

static void player_update(EntityID player, float delta);
static void player_take_damage(EntityID player, float damage);
static void player_die(EntityID player);

static EntityInterface player_interface = {
	.update = player_update,
	.take_damage = player_take_damage,
	.die = player_die
};

EntityID
ent_player_new(vec2 position)
{
	EntityID self_id = ent_new(ENTITY_PLAYER, &player_interface);
	process_init_components(self_id);
	
	self->sprite = gfx_scene_new_obj(0, SCENE_OBJECT_ANIMATED_SPRITE);
	vec2_dup(self_body->position, position);
	vec2_dup(self_body->half_size, (vec2){ ENTITY_SCALE, ENTITY_SCALE });
	vec2_dup(self_body->velocity, (vec2){ 0.0, 0.0 });
	self_body->is_static = false;
	self_body->collision_layer = PHX_LAYER_ENTITIES_BIT;
	self_body->collision_mask  = PHX_LAYER_ENTITIES_BIT | PHX_LAYER_MAP_BIT;
	self_body->entity = self_id;
	self_body->mass = 10.0;
	self_body->restitution = 0.01;
	self_body->damping = 5.0;

	vec2_dup(self_sprite->position, position);
	vec2_dup(self_sprite->half_size, (vec2){ ENTITY_SCALE, ENTITY_SCALE });
	vec4_dup(self_sprite->color, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	self_sprite->rotation = 0.0;
	self_sprite->fps = 0.0;
	self_sprite->time = 0.0;
	self_sprite->animation = ANIMATION_PLAYER_IDLE;
	self->fired = 0;

	MOB_COMPONENT.health = 10;
	MOB_COMPONENT.health_max = 10;

	EVENT_EMIT(EVENT_PLAYER_SPAWN, .player_id = self_id);

	return self_id;
}

void
player_update(EntityID self_id, float delta) 
{
	(void)delta;
	int mouse_x, mouse_y;

	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	int state = SDL_GetMouseState(&mouse_x, &mouse_y);

	if(keys[SDL_SCANCODE_W])
		self_body->accel[1] -= SPEED;
	if(keys[SDL_SCANCODE_S])
		self_body->accel[1] += SPEED;
	if(keys[SDL_SCANCODE_A]) {
		self_sprite->half_size[0] = -ENTITY_SCALE;
		self_body->accel[0] -= SPEED;
	}
	if(keys[SDL_SCANCODE_D]) {
		self_sprite->half_size[0] = ENTITY_SCALE;
		self_body->accel[0] += SPEED;
	}

	if(self_body->velocity[0] != 0 || self_body->velocity[1] != 0) {
		if(!self->moving) {
			self_sprite->animation = ANIMATION_PLAYER_MOVEMENT;
			self_sprite->fps = 5.0;
			self_sprite->time = 0.0;
			self->moving = true;
		}
	} else {
		self_sprite->animation = ANIMATION_PLAYER_IDLE;
		self_sprite->fps = 0.0;
		self_sprite->time = 0.0;
		self->moving = false;
	}

	if(SDL_BUTTON(SDL_BUTTON_LEFT) & state) {
		if(self->fired < 1) {
			vec2 mouse_pos;
			mouse_pos[0] = mouse_x, mouse_pos[1] = mouse_y;

			gfx_pixel_to_world(mouse_pos, mouse_pos);
			vec2_sub(mouse_pos, mouse_pos, self_body->position);
			vec2_normalize(mouse_pos, mouse_pos);
			vec2_mul(mouse_pos, mouse_pos, (vec2){ 40.0, 40.0 });
			ent_fireball_new(self_id, VEC2_DUP(self_body->position), mouse_pos);
			self->fired += 1;

			audio_sfx_play(AUDIO_MIXER_SFX, AUDIO_BUFFER_FIREBALL, 1.0);
		}
	} else {
		self->fired = 0;
	}
	vec2_dup(self_sprite->position, self_body->position);

	process_components(self_id);
}

void
player_take_damage(EntityID self_id, float health)
{
	self->mob.health += health;
}

void
player_die(EntityID self_id) 
{
	process_del_components(self_id);
	gfx_scene_del_obj(self->sprite);

	GLOBAL.player_id = 0;
}
