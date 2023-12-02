#include <stdbool.h>
#include <SDL2/SDL.h>
#include <assert.h>

#include "util.h"
#include "vecmath.h"
#include "physics.h"
#include "global.h"
#include "graphics.h"

#define PHYSICS_HZ 480
#define PHYSICS_TIME (1.0 / PHYSICS_HZ)
#define SCALE_FACTOR   (1)
#define DAMPING_FACTOR (1)

typedef unsigned int BodyGridNodeID;

typedef struct {
	BodyID next, prev;
	Body body;
	bool dead;
} BodyNode;

typedef struct {
	BodyGridNodeID next;
	BodyID body;
} BodyGridNode;

typedef struct {
	vec2 normal;
	vec2 pierce;
	BodyID self;
	BodyID target;
} Hit;

static bool have_to_test(BodyID self, BodyID target);

static BodyGridNodeID grid_to_id(BodyGridNode *node);
static BodyGridNode*  id_to_grid(BodyGridNodeID id);

static void calculate_grid();

static bool body_check_collision(BodyID self, BodyID target, Contact *contact);
static void update_body(BodyID body, float delta);
static void update_body_grid(BodyGridNodeID self_grid, BodyGridNodeID other_grid);

static void solve(Contact *contact);
static void body_grid_pos(vec2 position, int *grid_x, int *grid_y);

static float accumulator_time;

static ArrayBuffer grid_node_arena;
static int grid_size, grid_size_w, grid_size_h;
static BodyGridNodeID *grid_list;
static BodyGridNodeID *static_grid_list;

static void (*pre_solve_callback)(Contact *contact);

#define SYS_ID_TYPE   BodyID
#define SYS_NODE_TYPE BodyNode
#include "system.h"

void _sys_int_cleanup(BodyID id) { (void)id; }

void
phx_init()
{
	_sys_init();
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
phx_end()
{
	_sys_deinit();
}

void
phx_reset()
{
	_sys_reset();
}

BodyID phx_new()
{
	BodyID id = _sys_new();
	return id;
}

void
phx_del(BodyID id) 
{
	_sys_del(id);
}

Body *
phx_data(BodyID id)
{
	return &_sys_node(id)->body;
}

void
phx_update(float delta) 
{
	accumulator_time += delta;
	while(accumulator_time > PHYSICS_TIME) {
		_sys_cleanup();

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
		for(BodyID body = _sys_list; body; body = _sys_node(body)->next)
			update_body(body, PHYSICS_TIME * SCALE_FACTOR);

		accumulator_time -= PHYSICS_TIME;
	}
	for(BodyID body = _sys_list; body; body = _sys_node(body)->next)
		vec2_dup(phx_data(body)->accel, (vec2){ 0.0, 0.0 });
}

void
phx_draw()
{
	//TODO: DEBUG
	//BodyID body_id = _sys_list;
	//#define body phx_data(body_id)
//	gfx_debug_begin();
//	while(body_id) {
//		vec2 pos, size;
//
//		BodyID next = _sys_node(body_id)->next;
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
update_body(BodyID self, float delta) 
{
	#define SELF phx_data(self)
	vec2 accel;

	vec2_add_scaled(accel, SELF->accel, SELF->velocity, -SELF->damping * DAMPING_FACTOR * SCALE_FACTOR);
	vec2_add_scaled(SELF->velocity, SELF->velocity, accel, delta);
	vec2_add_scaled(SELF->position, SELF->position, SELF->velocity, delta);
	(void)body_grid_pos;

	#undef SELF
}

void
update_body_grid(BodyGridNodeID self_grid, BodyGridNodeID target_id_grid) 
{
	#define SELF phx_data(self)
	BodyID self = id_to_grid(self_grid)->body;

	for(; target_id_grid; target_id_grid = id_to_grid(target_id_grid)->next) {
		Contact contact = {0};
		BodyID target_id = id_to_grid(target_id_grid)->body;

		if(!(have_to_test(self, target_id) || have_to_test(target_id, self))) {
			continue;
		}

		if(body_check_collision(self, target_id, &contact)) {
			if(pre_solve_callback)
				pre_solve_callback(&contact);

			if(contact.active)
				solve(&contact);
		}
	}
	#undef SELF
}

bool 
body_check_collision(BodyID self_id, BodyID target_id, Contact *contact)
{
	#define SELF phx_data(self_id)
	#define TARGET phx_data(target_id)

	vec2 subbed_pos, added_size, min, max;

	vec2_sub(subbed_pos, TARGET->position,  SELF->position);
	vec2_add(added_size, TARGET->half_size, SELF->half_size);
	vec2_sub(min, subbed_pos, added_size);
	vec2_add(max, subbed_pos, added_size);

	if(!(min[0] < 0 && max[0] > 0 && min[1] < 0 && max[1] > 0)) {
	 	return false;
	}

	vec2 ht;
	ht[0] = added_size[0] - fabsf(subbed_pos[0]);
	ht[1] = added_size[1] - fabsf(subbed_pos[1]);
	contact->normal[0] = (ht[0] < ht[1]) * ((subbed_pos[0] > 0) - (subbed_pos[0] < 0));
	contact->normal[1] = (ht[1] < ht[0]) * ((subbed_pos[1] > 0) - (subbed_pos[1] < 0));
	contact->pierce[0] = contact->normal[0] * (ht[0]);
	contact->pierce[1] = contact->normal[1] * (ht[1]);
	contact->body1 = self_id;
	contact->body2 = target_id;
	contact->active = true;

	return true;

	#undef SELF
	#undef TARGET
}

void
solve(Contact *contact)
{
	#define SELF phx_data(contact->body1)
	#define TARGET phx_data(contact->body2)

	float j;
	vec2 vel_rel;

	float self_inertia = SELF->is_static ? 0 : 1.0 / SELF->mass;
	float target_inertia = TARGET->is_static ? 0 : 1.0 / TARGET->mass;

	vec2_sub(vel_rel, SELF->velocity, TARGET->velocity);
	j = vec2_dot(contact->normal, vel_rel);
	j *= -(1 + SELF->restitution + TARGET->restitution);
	j /= (self_inertia + target_inertia);

	vec2_add_scaled(SELF->velocity, SELF->velocity, contact->normal, j * self_inertia);
	vec2_add_scaled(TARGET->velocity, TARGET->velocity, contact->normal, -j * target_inertia);

	if(TARGET->is_static) {
		vec2_add_scaled(SELF->position, SELF->position, contact->pierce, -1.0f);
	} else if (SELF->is_static) {
		vec2_add_scaled(TARGET->position, TARGET->position, contact->pierce,  1.0f);
	} else {
		float total_mass = SELF->mass + TARGET->mass;

		vec2_add_scaled(SELF->position, SELF->position, contact->pierce, -(TARGET->mass / total_mass));
		vec2_add_scaled(TARGET->position, TARGET->position, contact->pierce, SELF->mass / total_mass);
	}

	#undef SELF
	#undef TARGET
}

void
body_grid_pos(vec2 position, int *grid_x, int *grid_y)
{
	*grid_x = floorf(position[0] / GRID_TILE_SIZE);
	*grid_y = floorf(position[1] / GRID_TILE_SIZE);
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
calculate_grid()
{
	memset(grid_list, 0, sizeof(grid_list[0]) * grid_size);
	memset(static_grid_list, 0, sizeof(static_grid_list[0]) * grid_size);
	arrbuf_clear(&grid_node_arena);
	(void)id_to_grid;
	
	for(BodyID body = _sys_list; body > 0; body = _sys_node(body)->next) {
		int grid_min_x = floorf((phx_data(body)->position[0] - phx_data(body)->half_size[0]) / GRID_TILE_SIZE);
		int grid_min_y = floorf((phx_data(body)->position[1] - phx_data(body)->half_size[1]) / GRID_TILE_SIZE);
		int grid_max_x = floorf((phx_data(body)->position[0] + phx_data(body)->half_size[0]) / GRID_TILE_SIZE);
		int grid_max_y = floorf((phx_data(body)->position[1] + phx_data(body)->half_size[1]) / GRID_TILE_SIZE);

		for(int x = grid_min_x; x <= grid_max_x; x++) {
			if(x < 0 || x >= grid_size_w)
				continue;

			for(int y = grid_min_y; y <= grid_max_y; y++) {
				if(y < 0 || y >= grid_size_h)
					continue;

				BodyGridNode *node = arrbuf_newptr(&grid_node_arena, sizeof(BodyGridNode));
				node->body = body;
				if(!phx_data(body)->is_static) {
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
have_to_test(BodyID self, BodyID target)
{
	return !!(phx_data(self)->collision_mask & phx_data(target)->collision_layer);
}

void 
phx_set_pre_solve(void (*pre)(Contact *contact))
{
	pre_solve_callback = pre;
}
