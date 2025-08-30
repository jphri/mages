#include <stdbool.h>
#include <SDL.h>
#include "vecmath.h"

#include "../global.h"
#include "graphics.h"
#include "physics.h"
#include "../entity.h"
#include "../id.h"
#include "audio.h"
#include "events.h"

#define SPEED 50

static void player_update(Entity *player, float delta);
static void player_take_damage(Entity *player, float damage);
static void player_die(Entity *player);

static EntityInterface player_interface = {
	.update = player_update,
	.take_damage = player_take_damage,
	.die = player_die
};

Player *
ent_player_new(vec2 position)
{
	Player *self = (Player*)&ent_new(ENTITY_PLAYER, &player_interface)->player;
	
	self->body = phx_new();
	self->sprite = gfx_scene_new_obj(1, SCENE_OBJECT_ANIMATED_SPRITE);
	vec2_dup(self->body->position, position);
	vec2_dup(self->body->half_size, (vec2){ ENTITY_SCALE, ENTITY_SCALE });
	vec2_dup(self->body->velocity, (vec2){ 0.0, 0.0 });
	self->body->is_static = false;
	self->body->collision_layer = PHX_LAYER_ENTITIES_BIT;
	self->body->collision_mask  = PHX_LAYER_ENTITIES_BIT | PHX_LAYER_MAP_BIT;
	self->body->entity = (Entity*)self;
	self->body->mass = 10.0;
	self->body->restitution = 0.01;
	self->body->damping = 5.0;

	vec2_dup(self->sprite->position, position);
	vec2_dup(self->sprite->half_size, (vec2){ ENTITY_SCALE, ENTITY_SCALE });
	vec4_dup(self->sprite->color, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	self->sprite->rotation = 0.0;
	self->sprite->fps = 0.0;
	self->sprite->time = 0.0;
	self->sprite->animation = ANIMATION_PLAYER_IDLE;
	self->fired = 0;

	self->mob.health = 10;
	self->mob.health_max = 10;
	self->moving = false;

	EVENT_EMIT(EVENT_PLAYER_SPAWN, .player = (Entity*)self);

	return self;
}

void
player_update(Entity *self_player, float delta) 
{
	(void)delta;
	int mouse_x, mouse_y;

	Player *self = &self_player->player;

	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	int state = SDL_GetMouseState(&mouse_x, &mouse_y);

	self->moving = false;
	if(keys[SDL_SCANCODE_W]) {
		self->body->accel[1] -= SPEED;
		self->moving = true;
	}
	if(keys[SDL_SCANCODE_S]) {
		self->body->accel[1] += SPEED;
		self->moving = true;
	}
	if(keys[SDL_SCANCODE_A]) {
		self->sprite->half_size[0] = -ENTITY_SCALE;
		self->body->accel[0] -= SPEED;
		self->moving = true;
	}
	if(keys[SDL_SCANCODE_D]) {
		self->sprite->half_size[0] = ENTITY_SCALE;
		self->body->accel[0] += SPEED;
		self->moving = true;
	}

	if(self->moving) {
		if(self->sprite->animation != ANIMATION_PLAYER_MOVEMENT) {
			self->sprite->animation = ANIMATION_PLAYER_MOVEMENT;
			self->sprite->fps = 5.0;
			self->sprite->time = 0.0;
		}
	} else {
		self->sprite->animation = ANIMATION_PLAYER_IDLE;
		self->sprite->fps = 0.0;
		self->sprite->time = 0.0;
	}

	if(SDL_BUTTON(SDL_BUTTON_LEFT) & state) {
		if(self->fired < 1) {
			vec2 mouse_pos;
			mouse_pos[0] = mouse_x, mouse_pos[1] = mouse_y;

			gfx_pixel_to_world(mouse_pos, mouse_pos);
			vec2_sub(mouse_pos, mouse_pos, self->body->position);
			vec2_normalize(mouse_pos, mouse_pos);
			vec2_mul(mouse_pos, mouse_pos, (vec2){ 40.0, 40.0 });
			ent_fireball_new(self_player, VEC2_DUP(self->body->position), mouse_pos);
			self->fired += 1;

			audio_sfx_play(AUDIO_MIXER_SFX, AUDIO_BUFFER_FIREBALL, 1.0);
		}
	} else {
		self->fired = 0;
	}
	vec2_dup(self->sprite->position, self->body->position);
	mob_process(self_player, &self->mob);
}

void
player_take_damage(Entity *self, float health)
{
	self->player.mob.health += health;
}

void
player_die(Entity *self_id) 
{
	Player *pl = &self_id->player;
	gfx_scene_del_obj(pl->sprite);
	phx_del(pl->body);

	GLOBAL.player = NULL;
}
