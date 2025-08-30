#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "vecmath.h"

#include "../graphics.h"
#include "../physics.h"
#include "../entity.h"
#include "../id.h"

#define SELF      ENT_DATA(ENTITY_DAMAGE_NUMBER, self)

static void damage_number_update(Entity *self, float delta);
static void damage_number_die(Entity *self);

static EntityInterface dn_interface = {
	.update = damage_number_update,
	.die = damage_number_die
};

DamageNumber * 
ent_damage_number(vec2 position, float damage)
{
	(void)damage;
	DamageNumber *self = (DamageNumber*)ent_new(ENTITY_DAMAGE_NUMBER, &dn_interface);
	self->text = gfx_scene_new_obj(1, SCENE_OBJECT_TEXT);

	vec2_dup(self->position, position);

	vec4_dup(self->text->color, (vec4){ 1.0, 1.0, 0.0, 1.0 });
	vec2_dup(self->text->char_size, (vec2){ 0.025, 0.025 });
	vec2_dup(self->text->position, position);
	self->text->text_ptr = self->damage_str;
	memset(self->damage_str, 0, sizeof(self->damage_str));
	snprintf(self->damage_str, sizeof(self->damage_str), "%c%0.2f", damage > 0 ? '+' : '-', fabsf(damage));

	self->time = 0.0; 
	self->max_time = 3.0;
	return self;
}

void
damage_number_update(Entity *ent, float delta) 
{
	vec2 position;
	DamageNumber *self = &ent->damage_number;

	self->time += delta;
	if(self->time > self->max_time) {
		ent_del((Entity*)self);
		return;
	}
	float clamped_time = fmin(self->time / self->max_time, 1.0);

	position[0] = clamped_time;
	position[1] = 3 * (clamped_time + 1.7724453880162288);
	position[1] = sinf(position[1] * position[1]);
	position[1] = -fabsf(position[1]) * (1 - clamped_time) * (1 - clamped_time) * (1.0 - clamped_time);
	vec2_add(position, position, self->position);
	vec2_dup(self->text->position, position);
}

void
damage_number_die(Entity* self) 
{
	gfx_scene_del_obj((SceneObject*)self->damage_number.text);
}
