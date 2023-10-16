#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "vecmath.h"

typedef struct {
	vec2 position;
	vec2 half_size;
	vec2 velocity;
} Body;

typedef struct {
	vec2 normal;
	vec2 pierce;
} Hit;

static bool body_check_collision(Body *self, Body *target, Hit *hit);
static void body_control(Body *body, float delta);
static void update_body(Body *body, float delta);
static void render_body(Body *body);

static SDL_Window *window;
static SDL_Renderer *renderer;

int
main()
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		return -1;

	window = SDL_CreateWindow("hello",
							  SDL_WINDOWPOS_UNDEFINED,
							  SDL_WINDOWPOS_UNDEFINED,
							  800, 600,
							  SDL_WINDOW_OPENGL);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	Body body = (Body){
		.position = { 400, 300 },
		.half_size = { 15, 15 },
	};
	Body test_body = (Body) {
		.position = { 300, 300 },
		.half_size = { 15, 15 },
	};

	Uint64 prev_time = SDL_GetPerformanceCounter();
	while(true) {
		Hit hit;
		SDL_Event event;
		Uint64 curr_time = SDL_GetPerformanceCounter();
		float delta = (float)(curr_time - prev_time) / SDL_GetPerformanceFrequency();
		prev_time = curr_time;

		update_body(&body, delta);
		body_control(&body, delta);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		render_body(&body);
		render_body(&test_body);

		//printf("%f %f\n", body.position[0], body.position[1]);

		if(body_check_collision(&body, &test_body, &hit)) {
			printf("HIT! NORMAL: %f %f | PIERCE: %f %f\n", hit.normal[0], hit.normal[1], hit.pierce[0], hit.pierce[1]);

			vec2_add(body.position, hit.pierce, body.position);
		}

		SDL_RenderPresent(renderer);
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				goto end_loop;
			}
		}
	}
end_loop:

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}

void
update_body(Body *body, float delta) 
{
	vec2_add_scaled(body->position, body->position, body->velocity, delta);
}

void 
render_body(Body *body)
{
	vec2 pos;
	vec2 size;

	vec2_sub(pos, body->position, body->half_size);
	vec2_mul(size, body->half_size, (vec2){ 2, 2 });
	
	SDL_RenderDrawRectF(renderer, &(SDL_FRect){
		.x = pos[0], .y = pos[1],
		.w = size[0], .h = size[1]
	});
}

void
body_control(Body *b, float delta) 
{
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	
	vec2_dup(b->velocity, (vec2){ 0.0, 0.0 });
	if(keys[SDL_SCANCODE_W])
		b->velocity[1] -= 100;
	if(keys[SDL_SCANCODE_S])
		b->velocity[1] += 100;
	if(keys[SDL_SCANCODE_A])
		b->velocity[0] -= 100;
	if(keys[SDL_SCANCODE_D])
		b->velocity[0] += 100;
}

bool 
body_check_collision(Body *self, Body *target, Hit *hit)
{
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
}
