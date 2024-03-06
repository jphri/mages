#include <stdbool.h>
#include <SDL.h>
#include <assert.h>

#include "game_objects.h"
#include "util.h"
#include "vecmath.h"
#include "physics.h"
#include "global.h"
#include "graphics.h"

#define PHYSICS_HZ 480
#define PHYSICS_TIME (1.0 / PHYSICS_HZ)
#define SCALE_FACTOR   (1)
#define DAMPING_FACTOR (1)
#define EPSILON 0.01

typedef unsigned int BodyGridNodeID;

typedef struct {
	BodyGridNodeID next;
	Body *body;
} BodyGridNode;

typedef struct {
	vec2 normal;
	vec2 pierce;
	Body *self, *target;
} Hit;

static bool have_to_test(Body *self, Body *target);

static BodyGridNodeID grid_to_id(BodyGridNode *node);
static BodyGridNode*  id_to_grid(BodyGridNodeID id);

static void calculate_grid(void);

static bool body_check_collision(Body * self, Body * target, Contact *contact);
static void update_body(Body * body, float delta);
static void update_body_grid(BodyGridNodeID self_grid, BodyGridNodeID other_grid);

static void solve(Contact *contact);

static float accumulator_time;
static ArrayBuffer grid_node_arena;
static int grid_size, grid_size_w, grid_size_h;
static BodyGridNodeID *grid_list;
static BodyGridNodeID *static_grid_list;
static ObjectPool objects;

static void (*pre_solve_callback)(Contact *contact);

void
phx_init(void)
{
	objpool_init(&objects, sizeof(Body), DEFAULT_ALIGNMENT);
	arrbuf_init(&grid_node_arena);
	accumulator_time = 0;

	phx_set_grid_size(128, 128);
}

void
phx_set_grid_size(int w, int h)
{
	if(grid_list) free(grid_list);
	if(static_grid_list) free(static_grid_list);

	grid_size = w * h;
	grid_size_w = w;
	grid_size_h = h;

	grid_list = calloc(sizeof(grid_list[0]), grid_size);
	static_grid_list = calloc(sizeof(grid_list[0]), grid_size);
}

void
phx_end(void)
{
	arrbuf_free(&grid_node_arena);
	objpool_terminate(&objects);
}

void
phx_reset(void)
{
	objpool_reset(&objects);
}

Body * 
phx_new(void)
{
	Body *body = objpool_new(&objects);
	memset(body, 0, sizeof(*body));
	return body;
}

void
phx_del(Body *body)
{
	objpool_free(body);
}

void
phx_update(float delta)
{
	accumulator_time += delta;
	while(accumulator_time > PHYSICS_TIME) {
		objpool_clean(&objects);
		calculate_grid();

		for(int i = 0; i < grid_size; i++) {
			BodyGridNodeID node_id = grid_list[i];
			BodyGridNodeID static_node_id = static_grid_list[i];
			while(node_id) {
				update_body_grid(node_id, id_to_grid(node_id)->next);
				update_body_grid(node_id, static_node_id);
				node_id = id_to_grid(node_id)->next;
			}
		}
		for(Body *body = objpool_begin(&objects); body; body = objpool_next(body))
			update_body(body, PHYSICS_TIME * SCALE_FACTOR);

		accumulator_time -= PHYSICS_TIME;
	}
	for(Body *body = objpool_begin(&objects); body; body = objpool_next(body))
		vec2_dup(body->accel, (vec2){ 0.0, 0.0 });
}

void
phx_draw(void)
{
	//TODO: DEBUG
	//Body * body_id = _sys_list;
	//#define body phx_data(body_id)
//	gfx_debug_begin();
//	while(body_id) {
//		vec2 pos, size;
//
//		Body * next = _sys_node(body_id)->next;
//		vec2_sub(pos, body->position, body->half_size);
//		vec2_mul(size, body->half_size, (vec2){ 2, 2 });
//
//		gfx_debug_set_color((vec4){ 1.0, 1.0, 1.0, 1.0 });
//		gfx_debug_quad(body->position, body->half_size);
//
//		body_id = next;
//	}
//	gfx_debug_end();
}

void
update_body(Body * self, float delta)
{
	vec2 accel;
	
	if(!self->is_static) {
		vec2_add_scaled(accel, self->accel, self->velocity, -self->damping * DAMPING_FACTOR * SCALE_FACTOR);
		vec2_add_scaled(self->velocity, self->velocity, accel, delta);
		vec2_add_scaled(self->position, self->position, self->velocity, delta);
	} else {
		vec2_dup(self->velocity, (vec2){ 0.0, 0.0 });
		vec2_dup(self->accel, (vec2){ 0.0, 0.0 });
	}
}

void
update_body_grid(BodyGridNodeID self_grid, BodyGridNodeID target_id_grid)
{
	Body* self = id_to_grid(self_grid)->body;

	if(objpool_is_dead(self))
		return;

	for(; target_id_grid; target_id_grid = id_to_grid(target_id_grid)->next) {
		Contact contact = {0};
		Body *target = id_to_grid(target_id_grid)->body;

		if(objpool_is_dead(target))
			continue;

		if(!(have_to_test(self, target) || have_to_test(target, self))) {
			continue;
		}

		if(body_check_collision(self, target, &contact)) {
			if(self->pre_solve)
				self->pre_solve(self, target, &contact);

			if(target->pre_solve)
				target->pre_solve(target, self, &contact);

			if(contact.active)
				solve(&contact);
			
			if(objpool_is_dead(self))
				return;
		}
	}
}

bool
body_check_collision(Body *self, Body *target, Contact *contact)
{
	vec2 subbed_pos, added_size, min, max;

	vec2_sub(subbed_pos, target->position,  self->position);
	vec2_add(added_size, target->half_size, self->half_size);
	vec2_sub(min, subbed_pos, added_size);
	vec2_add(max, subbed_pos, added_size);

	if(!(min[0] < -EPSILON && max[0] > EPSILON && min[1] < -EPSILON && max[1] > EPSILON)) {
	 	return false;
	}

	vec2 ht;
	ht[0] = added_size[0] - fabsf(subbed_pos[0]);
	ht[1] = added_size[1] - fabsf(subbed_pos[1]);
	contact->normal[0] = (ht[0] < ht[1]) * ((subbed_pos[0] > 0) - (subbed_pos[0] < 0));
	contact->normal[1] = (ht[1] < ht[0]) * ((subbed_pos[1] > 0) - (subbed_pos[1] < 0));
	contact->pierce[0] = contact->normal[0] * (ht[0]);
	contact->pierce[1] = contact->normal[1] * (ht[1]);
	contact->body1 = self;
	contact->body2 = target;
	contact->active = true;

	return true;
}

void
solve(Contact *contact)
{
	float j;
	vec2 vel_rel;

	Body *self = contact->body1;
	Body *target = contact->body2;

	float self_inertia = self->is_static ? 0 : 1.0 / self->mass;
	float target_inertia = target->is_static ? 0 : 1.0 / target->mass;

	vec2_sub(vel_rel, VEC2_DUP(self->velocity), VEC2_DUP(target->velocity));
	j = vec2_dot(contact->normal, vel_rel);
	j *= -(1 + self->restitution + target->restitution);
	j /= (self_inertia + target_inertia);
	
	vec2_add_scaled(self->velocity, self->velocity, contact->normal, j * self_inertia);
	vec2_add_scaled(target->velocity, target->velocity, contact->normal, -j * target_inertia);

	if(target->is_static) {
		vec2_add_scaled(self->position, self->position, contact->pierce, -1.0f);
	} else if (self->is_static) {
		vec2_add_scaled(target->position, target->position, contact->pierce, 1.0f);
	} else {
		float total_mass = self->mass + target->mass;
		vec2_add_scaled(self->position, self->position, contact->pierce, -(target->mass / total_mass));
		vec2_add_scaled(target->position, target->position, contact->pierce, self->mass / total_mass);
	}
}

BodyGridNodeID
grid_to_id(BodyGridNode *node)
{
	return (node - (BodyGridNode*)grid_node_arena.data) + 1;
}

BodyGridNode *
id_to_grid(BodyGridNodeID id)
{
	return (BodyGridNode*)grid_node_arena.data + (id - 1);
}

void
calculate_grid(void)
{
	memset(grid_list, 0, sizeof(grid_list[0]) * grid_size);
	memset(static_grid_list, 0, sizeof(static_grid_list[0]) * grid_size);
	arrbuf_clear(&grid_node_arena);

	for(Body *body = objpool_begin(&objects); body; body = objpool_next(body)) {
		int grid_min_x = floorf(body->position[0] - body->half_size[0]) / GRID_TILE_SIZE;
		int grid_min_y = floorf(body->position[1] - body->half_size[1]) / GRID_TILE_SIZE;
		int grid_max_x = floorf(body->position[0] + body->half_size[0]) / GRID_TILE_SIZE;
		int grid_max_y = floorf(body->position[1] + body->half_size[1]) / GRID_TILE_SIZE;

		for(int x = grid_min_x; x <= grid_max_x; x++) {
			if(x < 0 || x >= grid_size_w)
				continue;

			for(int y = grid_min_y; y <= grid_max_y; y++) {
				if(y < 0 || y >= grid_size_h)
					continue;

				BodyGridNode *node = arrbuf_newptr(&grid_node_arena, sizeof(BodyGridNode));
				node->body = body;
				if(!body->is_static) {
					node->next = grid_list[x + y * grid_size_w];
					grid_list[x + y * grid_size_w] = grid_to_id(node);
				} else {
					node->next = static_grid_list[x + y * grid_size_w];
					static_grid_list[x + y * grid_size_w] = grid_to_id(node);
				}
			}
		}
	}
}

bool
have_to_test(Body *self, Body *target)
{
	return !!(self->collision_mask & target->collision_layer);
}

void
phx_set_pre_solve(void (*pre)(Contact *contact))
{
	pre_solve_callback = pre;
}
