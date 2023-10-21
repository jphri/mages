#include <stdio.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "global.h"
#include "vecmath.h"
#include "physics.h"
#include "entity.h"
#include "graphics.h"

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
							  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	GLOBAL.renderer = SDL_CreateRenderer(GLOBAL.window, -1, SDL_RENDERER_ACCELERATED);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	GLOBAL.glctx = SDL_GL_CreateContext(GLOBAL.window);
	SDL_GL_MakeCurrent(GLOBAL.window, GLOBAL.glctx);

	if(glewInit() != GLEW_OK)
		return -1;

	gfx_init();
	gfx_scene_setup();
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
	gfx_scene_set_tilemap(0, TERRAIN_NORMAL, 3, 4, (int[]){
		1, 1, 1,
		1, 2, 1,
		1, 0, 1,
		1, 1, 1
	});

	ent_player_new((vec2){ 0.0, 0.0 });

	Uint64 prev_time = SDL_GetPerformanceCounter();
	while(true) {
		int w, h;
		SDL_Event event;
		Uint64 curr_time = SDL_GetPerformanceCounter();
		float delta = (float)(curr_time - prev_time) / SDL_GetPerformanceFrequency();
		prev_time = curr_time;

		phx_update(delta);
		ent_update(delta);
		ent_render();

		SDL_GetWindowSize(GLOBAL.window, &w, &h);

		gfx_make_framebuffers(w, h);
		gfx_clear_framebuffers();
		gfx_scene_draw();
		gfx_render_present();

		SDL_GL_SwapWindow(GLOBAL.window);
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				goto end_loop;
			}
		}
	}

end_loop:
	phx_end();
	ent_end();
	gfx_end();
	gfx_scene_cleanup();

	SDL_DestroyRenderer(GLOBAL.renderer);
	SDL_DestroyWindow(GLOBAL.window);
	SDL_Quit();
	return 0;
}
