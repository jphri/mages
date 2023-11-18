#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "../graphics.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"
#include "../id.h"

#define SELF      ENT_DATA(ENTITY_DAMAGE_NUMBER, self)

EntityID 
ent_damage_number(vec2 position, float damage)
{
	EntityID self = ent_new(ENTITY_DAMAGE_NUMBER);
	SELF->text_id = gfx_scene_new_obj(0, SCENE_OBJECT_TEXT);

	vec2_dup(SELF->position, position);

	vec4_dup(gfx_scene_text(SELF->text_id)->color, (vec4){ 1.0, 1.0, 0.0, 1.0 });
	vec2_dup(gfx_scene_text(SELF->text_id)->char_size, (vec2){ 0.35, 0.35 });
	vec2_dup(gfx_scene_text(SELF->text_id)->position, position);
	gfx_scene_text(SELF->text_id)->text_ptr = SELF->damage_str;
	snprintf(SELF->damage_str, sizeof(SELF->damage_str), "%c%0.2f", damage > 0 ? '+' : '-', fabsf(damage));

	SELF->time = 0.0; 
	SELF->max_time = 3.0;
	return self;
}

void
ENTITY_DAMAGE_NUMBER_update(EntityID self, float delta) 
{
	vec2 position;

	SELF->time += delta;
	if(SELF->time > SELF->max_time) {
		ent_del(self);
		return;
	}
	float clamped_time = fmin(SELF->time / SELF->max_time, 1.0);

	position[0] = clamped_time;
	position[1] = 3 * (clamped_time + 1.7724453880162288);
	position[1] = sinf(position[1] * position[1]);
	position[1] = -fabsf(position[1]) * (1 - clamped_time) * (1 - clamped_time) * (1.0 - clamped_time);
	vec2_add(position, position, SELF->position);
	vec2_dup(gfx_scene_text(SELF->text_id)->position, position);
}

void
ENTITY_DAMAGE_NUMBER_render(EntityID self)
{
	(void)self;
}

void
ENTITY_DAMAGE_NUMBER_del(EntityID self) 
{
	gfx_scene_del_obj(SELF->text_id);
}
