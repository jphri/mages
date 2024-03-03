#include "../physics.h"
#include "../entity.h"
#include "../graphics.h"
#include <math.h>
#include <stdbool.h>

#define DOOR(ID) ENT_DATA(ENTITY_DOOR, ID)

#define M_PI 3.1415926535
#define EPSLON 0.001
#define DRAG 100

static void door_update(Entity *, float);
static void door_die(Entity *);

static EntityInterface door_int = {
	.die = door_die,
	.update = door_update,
};

Door *
ent_door_new(vec2 position, enum DoorDir door_dir)
{
	Door *door = &ent_new(ENTITY_DOOR, &door_int)->door;

	door->openness = 0;
	door->openness_speed = 0;
	door->open = false;
	door->dir = door_dir;
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

	switch(door_dir) {
	case DOOR_DIR_LEFT:
		door->door_angle = 0;
		door->body->half_size[0] = ENTITY_SCALE / 8;
		door->body->half_size[1] = ENTITY_SCALE * 2;
		vec2_sub(door->line->p1, position, (vec2){ 0.0, ENTITY_SCALE * 2.0 });
		vec2_add(door->line->p2, position, (vec2){ 0.0, ENTITY_SCALE * 2.0 });
		break;
	case DOOR_DIR_RIGHT:
		door->door_angle = M_PI;
		door->body->half_size[0] = ENTITY_SCALE / 8;
		door->body->half_size[1] = ENTITY_SCALE * 2;
		vec2_add(door->line->p1, position, (vec2){ 0.0, ENTITY_SCALE * 2.0 });
		vec2_sub(door->line->p2, position, (vec2){ 0.0, ENTITY_SCALE * 2.0 });
		break;
	case DOOR_DIR_UP:
		door->door_angle = M_PI / 2.0;
		door->body->half_size[1] = ENTITY_SCALE / 8;
		door->body->half_size[0] = ENTITY_SCALE * 2;
		vec2_sub(door->line->p1, position, (vec2){ ENTITY_SCALE * 2.0, 0.0 });
		vec2_add(door->line->p2, position, (vec2){ ENTITY_SCALE * 2.0, 0.0 });
		break;
	case DOOR_DIR_DOWN:
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
	}
}

void
ent_door_close(Door *door)
{
	if(door->open) {
		door->open = false;
		door->body->active = 1;
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
	dist *= 0.1 * 1000.0;
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
