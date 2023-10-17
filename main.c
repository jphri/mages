#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "global.h"
#include "vecmath.h"
#include "physics.h"
#include "entity.h"

Global GLOBAL;

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
	ent_init();

	BodyID body2 = phx_new();
	*phx_data(body2) = (Body) {
		.position = { 330, 300 },
		.half_size = { 45, 15 },
		.velocity = { 0.0, 0.0 },
		.solve_layer     =   0,
		.solve_mask      = 0x1,
		.collision_layer =   0,
		.collision_mask  = 0x1,
		.is_static = true
	};
	ent_player_new((vec2){ 400, 300 });

	Uint64 prev_time = SDL_GetPerformanceCounter();
	while(true) {
		SDL_Event event;
		Uint64 curr_time = SDL_GetPerformanceCounter();
		float delta = (float)(curr_time - prev_time) / SDL_GetPerformanceFrequency();
		prev_time = curr_time;

		phx_update(delta);
		ent_update(delta);

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

	phx_end();
	SDL_DestroyRenderer(GLOBAL.renderer);
	SDL_DestroyWindow(GLOBAL.window);
	SDL_Quit();
	return 0;
}
