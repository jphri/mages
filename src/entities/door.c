#include "../physics.h"
#include "../entity.h"
#include "../graphics.h"
#include "../audio.h"
#include <math.h>
#include <assert.h>
#include <stdbool.h>

#define DOOR(ID) ENT_DATA(ENTITY_DOOR, ID)

#ifndef M_PI
#define M_PI 3.1415926535
#endif

#define EPSLON 0.001
#define DRAG 100

static Rectangle door_hover_rect(Entity *);
static void door_update(Entity *, float);
static void door_die(Entity *);
static bool door_mouse_hovered(Entity *, vec2 mouse_pos);
static void door_mouse_interact(Entity *, Player *, vec2 mouse_pos);

static EntityInterface door_int = {
	.die = door_die,
	.update = door_update,
	.mouse_hovered = door_mouse_hovered,
	.mouse_interact = door_mouse_interact
};

Door *
ent_door_new(vec2 position, Direction direction)
{
	Door *door = &ent_new(ENTITY_DOOR, &door_int)->door;

	door->openness = 0;
	door->openness_speed = 0;
	door->open = false;
	door->direction = direction;
	door->line = gfx_scene_new_obj(1, SCENE_OBJECT_LINE);
	vec2_dup(door->line->p1, position);
	vec2_add(door->line->p2, position, (vec2){ 0, ENTITY_SCALE * 4.0 });
	door->line->color[0] = 0.345;
	door->line->color[1] = 0.192;
	door->line->color[2] = 0.149;
	door->line->color[3] = 1.000;
	door->line->thickness = ENTITY_SCALE / 8;

	door->body = phx_new();
	vec2_dup(door->body->position, position);
	door->body->half_size[0] = ENTITY_SCALE / 8;
	door->body->half_size[1] = ENTITY_SCALE * 2;
	door->body->is_static = true;
	door->body->no_update = true;
	door->body->collision_layer = PHX_LAYER_ENTITIES_BIT;
	door->body->solve_layer = PHX_LAYER_ENTITIES_BIT;
	door->body->entity = (Entity*)door;

	switch(direction) {
	case DIR_RIGHT:
		door->door_angle = 0;
		door->body->half_size[0] = ENTITY_SCALE / 8;
		door->body->half_size[1] = ENTITY_SCALE * 2;
		vec2_sub(door->line->p1, position, (vec2){ 0.0, ENTITY_SCALE * 2.0 });
		vec2_add(door->line->p2, position, (vec2){ 0.0, ENTITY_SCALE * 2.0 });
		break;
	case DIR_LEFT:
		door->door_angle = M_PI;
		door->body->half_size[0] = ENTITY_SCALE / 8;
		door->body->half_size[1] = ENTITY_SCALE * 2;
		vec2_add(door->line->p1, position, (vec2){ 0.0, ENTITY_SCALE * 2.0 });
		vec2_sub(door->line->p2, position, (vec2){ 0.0, ENTITY_SCALE * 2.0 });
		break;
	case DIR_UP:
		door->door_angle = M_PI / 2.0;
		door->body->half_size[1] = ENTITY_SCALE / 8;
		door->body->half_size[0] = ENTITY_SCALE * 2;
		vec2_sub(door->line->p1, position, (vec2){ ENTITY_SCALE * 2.0, 0.0 });
		vec2_add(door->line->p2, position, (vec2){ ENTITY_SCALE * 2.0, 0.0 });
		break;
	case DIR_DOWN:
		door->door_angle = M_PI + M_PI / 2.0;
		door->body->half_size[1] = ENTITY_SCALE / 8;
		door->body->half_size[0] = ENTITY_SCALE * 2;
		vec2_add(door->line->p1, position, (vec2){ ENTITY_SCALE * 2.0, 0.0 });
		vec2_sub(door->line->p2, position, (vec2){ ENTITY_SCALE * 2.0, 0.0 });
		break;
	}

	return door;
}

void
ent_door_open(Door *door)
{
	if(!door->open) {
		door->open = true;
		door->body->active = 0;
		audio_sfx_play(AUDIO_MIXER_SFX, AUDIO_BUFFER_DOOR_OPEN, 1.0);
	}
}

void
ent_door_close(Door *door)
{
	if(door->open) {
		door->open = false;
		door->body->active = 1;
		audio_sfx_play(AUDIO_MIXER_SFX, AUDIO_BUFFER_DOOR_CLOSE, 1.0);
	}
}

bool
ent_door_is_open(Door *door)
{
	return door->open;
}

void
door_update(Entity *door_ent, float delta)
{
	Door *door = &door_ent->door;
	
	float dist = (float)door->open - door->openness;
	dist *= 0.25 * 1000.0;
	dist -= DRAG * 0.1 * door->openness_speed; 
	door->openness_speed += (dist * delta) * 0.1;
	door->openness += (door->openness_speed * delta);
	if(door->openness > 1.1) {
		door->openness_speed = 0.0;
		door->openness = 1.1;
	}
	if(door->openness < 0.0) {
		door->openness_speed = 0.0;
		door->openness = 0.0;
	}

	door->line->p2[0] = sinf(door->openness * M_PI * 0.5 + door->door_angle) * ENTITY_SCALE * 4.0 + door->line->p1[0];
	door->line->p2[1] = cosf(door->openness * M_PI * 0.5 + door->door_angle) * ENTITY_SCALE * 4.0 + door->line->p1[1];
}

void
door_die(Entity *door_ent)
{
	gfx_scene_del_obj(door_ent->door.line);
	phx_del(door_ent->door.body);
}

bool 
door_mouse_hovered(Entity *door_ent, vec2 mouse_pos)
{
	Rectangle rect = door_hover_rect(door_ent);
	return rect_contains_point(&rect, mouse_pos);
}

Rectangle
door_hover_rect(Entity *door_ent)
{
	Door *door = &door_ent->door;
	Rectangle rect;

	vec2_dup(rect.position, door->body->position);
	switch(door->direction) {
	case DIR_RIGHT:
		rect.half_size[0] = ENTITY_SCALE;
		rect.half_size[1] = ENTITY_SCALE * 2;
		if(door->open) {
			rect.position[0] += ENTITY_SCALE * 2;
			rect.position[1] -= ENTITY_SCALE * 2;

			rect.half_size[1] = ENTITY_SCALE;
			rect.half_size[0] = ENTITY_SCALE * 2;
		}
		break;
	case DIR_LEFT:
		rect.half_size[0] = ENTITY_SCALE;
		rect.half_size[1] = ENTITY_SCALE * 2;

		if(door->open) {
			rect.position[0] -= ENTITY_SCALE * 2;
			rect.position[1] += ENTITY_SCALE * 2;

			rect.half_size[1] = ENTITY_SCALE;
			rect.half_size[0] = ENTITY_SCALE * 2;
		}
		break;
	case DIR_UP:
		rect.half_size[1] = ENTITY_SCALE;
		rect.half_size[0] = ENTITY_SCALE * 2;
		
		if(door->open) {
			rect.position[1] -= ENTITY_SCALE * 2;
			rect.position[0] -= ENTITY_SCALE * 2;

			rect.half_size[1] = ENTITY_SCALE;
			rect.half_size[0] = ENTITY_SCALE * 2;
		}
		break;
	case DIR_DOWN:
		rect.half_size[1] = ENTITY_SCALE;
		rect.half_size[0] = ENTITY_SCALE * 2;

		if(door->open) {
			rect.position[1] += ENTITY_SCALE * 2;
			rect.position[0] += ENTITY_SCALE * 2;

			rect.half_size[0] = ENTITY_SCALE;
			rect.half_size[1] = ENTITY_SCALE * 2;
		}

		break;
	default:
		assert(0 && "wrong door dir");
	}

	return rect;
}

void 
door_mouse_interact(Entity *ent, Player *player, vec2 mouse_pos)
{
	(void)player;
	(void)mouse_pos;

	Door *door = &ent->door;
	if(door->open) {
		ent_door_close(door);
	} else {
		ent_door_open(door);
	}
}
