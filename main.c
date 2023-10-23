#include <stdio.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "global.h"
#include "vecmath.h"
#include "physics.h"
#include "entity.h"
#include "graphics.h"
#include "map.h"

Global GLOBAL;
static Map *map;

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

	map = map_load("maps/test_map_2.map");
	map_set_gfx_scene(map);
	map_set_phx_scene(map);
	map_free(map);

	EntityID player_id = ent_player_new((vec2){ 15.0, 15.0 });

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
		
		#define PLAYER ((EntityPlayer*)ent_data(player_id))
		#define PLAYER_BODY phx_data(PLAYER->body)
		vec2 offset;
		vec2_add_scaled(offset, (vec2){ 0.0, 0.0 }, PLAYER_BODY->position, -32);
		vec2_add(offset, offset, (vec2){ 400, 300 });
		gfx_set_camera(offset, (vec2){ 32.0, 32.0 });

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
