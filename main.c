#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "global.h"
#include "vecmath.h"
#include "physics.h"

Global GLOBAL;

static void body_control(BodyID id, float delta);

int
main()
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		return -1;

	GLOBAL.window = SDL_CreateWindow("hello",
							  SDL_WINDOWPOS_UNDEFINED,
							  SDL_WINDOWPOS_UNDEFINED,
							  800, 600,
							  SDL_WINDOW_OPENGL);
	GLOBAL.renderer = SDL_CreateRenderer(GLOBAL.window, -1, SDL_RENDERER_ACCELERATED);

	phx_init();

	BodyID body = phx_new();
	*phx_data(body) = (Body) {
		.position = { 400, 300 },
		.half_size = { 15, 15 },
		.velocity = { 0.0, 0.0 }
	};

	BodyID body2 = phx_new();
	*phx_data(body2) = (Body) {
		.position = { 330, 300 },
		.half_size = { 45, 15 },
		.velocity = { 0.0, 0.0 }
	};

	Uint64 prev_time = SDL_GetPerformanceCounter();
	while(true) {
		SDL_Event event;
		Uint64 curr_time = SDL_GetPerformanceCounter();
		float delta = (float)(curr_time - prev_time) / SDL_GetPerformanceFrequency();
		prev_time = curr_time;

		phx_update(delta);
		body_control(body, delta);

		SDL_SetRenderDrawColor(GLOBAL.renderer, 0, 0, 0, 255);
		SDL_RenderClear(GLOBAL.renderer);
		SDL_SetRenderDrawColor(GLOBAL.renderer, 255, 0, 0, 255);
		phx_draw();

		SDL_RenderPresent(GLOBAL.renderer);
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				goto end_loop;
			}
		}
	}
end_loop:

	SDL_DestroyRenderer(GLOBAL.renderer);
	SDL_DestroyWindow(GLOBAL.window);
	SDL_Quit();
	return 0;
}

void
body_control(BodyID b_id, float delta) 
{
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	#define b phx_data(b_id)
	
	vec2_dup(b->velocity, (vec2){ 0.0, 0.0 });
	if(keys[SDL_SCANCODE_W])
		b->velocity[1] -= 100;
	if(keys[SDL_SCANCODE_S])
		b->velocity[1] += 100;
	if(keys[SDL_SCANCODE_A])
		b->velocity[0] -= 100;
	if(keys[SDL_SCANCODE_D])
		b->velocity[0] += 100;
	#undef b
}

