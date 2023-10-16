#include <stdbool.h>
#include <SDL2/SDL.h>

#include "util.h"
#include "vecmath.h"
#include "physics.h"
#include "global.h"

typedef struct {
	BodyID next, prev;
	Body body;
} BodyNode;

typedef struct {
	vec2 normal;
	vec2 pierce;
} Hit;

static bool body_check_collision(BodyID self, BodyID target, Hit *hit);
static void update_body(BodyID body, float delta);

#define SYS_ID_TYPE   BodyID
#define SYS_NODE_TYPE BodyNode
#include "system.h"

void
phx_init()
{
	_sys_init();
}

void
phx_end()
{
	_sys_deinit();
}

BodyID phx_new()
{
	return _sys_new();
}

void
phx_del(BodyID id) 
{
	return _sys_del(id);
}

Body *
phx_data(BodyID id)
{
	return &_sys_node(id)->body;
}

void
phx_update(float delta) 
{
	BodyID body_id = _sys_list;
	while(body_id) {
		BodyID next = _sys_node(body_id)->next;
		update_body(body_id, delta);
		body_id = next;
	}
}

void
phx_draw()
{
	BodyID body_id = _sys_list;
	#define body phx_data(body_id)
	while(body_id) {
		vec2 pos, size;

		BodyID next = _sys_node(body_id)->next;
		vec2_sub(pos, body->position, body->half_size);
		vec2_mul(size, body->half_size, (vec2){ 2, 2 });
		
		SDL_RenderDrawRectF(GLOBAL.renderer, &(SDL_FRect){
			.x = pos[0], .y = pos[1],
			.w = size[0], .h = size[1]
		});
		body_id = next;
	}
}

void
update_body(BodyID self, float delta) 
{
	#define SELF phx_data(self)
	vec2_add_scaled(SELF->position, SELF->position, SELF->velocity, delta);

	BodyID target_id = _sys_list;
	while(target_id) {
		Hit hit;
		BodyID next = _sys_node(target_id)->next;

		if(target_id != self && body_check_collision(self, target_id, &hit))
			vec2_add(SELF->position, hit.pierce, SELF->position);

		target_id = next;
	}
	#undef SELF
}

bool 
body_check_collision(BodyID self_id, BodyID target_id, Hit *hit)
{
	#define self phx_data(self_id)
	#define target phx_data(target_id)

	vec2 subbed_pos, added_size, min, max;
	vec2 t_min, t_max;
	
	vec2_add(added_size, target->half_size, self->half_size);
	vec2_sub(subbed_pos, target->position, self->position);
	vec2_sub(min, subbed_pos, added_size);
	vec2_add(max, subbed_pos, added_size);

	if(!(min[0] < 0 && max[0] > 0 && min[1] < 0 && max[1] > 0))
		return false;

	vec2 delta;
	delta[0] = added_size[0] - fabs(subbed_pos[0]);
	delta[1] = added_size[1] - fabs(subbed_pos[1]);

	if(delta[0] < delta[1]) {
		if(subbed_pos[0] < 0)
			hit->normal[0] =  1.0;
		else 
			hit->normal[0] = -1.0;
		hit->normal[1] = 0.0;
	} else {
		if(subbed_pos[1] < 0)
			hit->normal[1] = -1.0;
		else 
			hit->normal[1] =  1.0;

		hit->normal[0] = 0.0;
	}

	hit->pierce[0] =  hit->normal[0] * (delta[0]);
	hit->pierce[1] = -hit->normal[1] * (delta[1]);
	return true;

	#undef self
	#undef target
}
