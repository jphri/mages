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
#include "ui.h"
#include "util.h"

Global GLOBAL;
static Map *map;
static ArrayBuffer label_ptr;
static int hello_count;
static UIObject label;

int
main()
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		return -1;

	arrbuf_init(&label_ptr);
	arrbuf_printf(&label_ptr, "Hello count: %d\n", hello_count);

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
	ui_init();

	UIObject window = ui_new_object(0, UI_WINDOW);
	ui_obj_set_size(window, (vec2){ 90, 35 });
	ui_obj_set_position(window, (vec2){ 800 - 90, 35 });
	ui_window_set_bg(window, (vec4){ 0.2, 0.2, 0.2, 1.0 });
	ui_window_set_border(window, (vec4){ 0.6, 0.6, 0.6, 1.0 });
	ui_window_set_border_size(window, (vec2){ 3, 3 });

	UIObject layout3 = ui_new_object(window, UI_LAYOUT);
	ui_obj_set_size(layout3, (vec2){ 70, 35 });
	ui_obj_set_position(layout3, (vec2) { 90, 35 });
	ui_layout_set_order(layout3, UI_LAYOUT_VERTICAL);

	label = ui_new_object(layout3, UI_LABEL);
	ui_label_set_color(label, (vec4){ 1.0, 1.0, 0.0, 1.0 });
	ui_label_set_border(label, (vec2){ 10.0, 10.0 });
	ui_label_set_text(label, "Hello");

	map = map_load("maps/test_map_2.map");
	map_set_gfx_scene(map);
	map_set_phx_scene(map);
	map_free(map);

	EntityID player_id = ent_player_new((vec2){ 15.0, 15.0 });
	ent_dummy_new((vec2){ 25.0, 15.0 });

	Uint64 prev_time = SDL_GetPerformanceCounter();

	(void)player_id;
	while(true) {
		int w, h;
		SDL_Event event;
		Uint64 curr_time = SDL_GetPerformanceCounter();
		float delta = (float)(curr_time - prev_time) / SDL_GetPerformanceFrequency();
		prev_time = curr_time;

		arrbuf_clear(&label_ptr);
		arrbuf_printf(&label_ptr, "FPS: %0.2f", 1.0/delta);
		ui_label_set_text(label, label_ptr.data);

		#define PLAYER ((EntityPlayer*)ent_data(player_id))
		#define PLAYER_BODY phx_data(PLAYER->body)
		vec2 offset;
		vec2_add_scaled(offset, (vec2){ 0.0, 0.0 }, PLAYER_BODY->position, -32);
		vec2_add(offset, offset, (vec2){ 400, 300 });
		gfx_set_camera(offset, (vec2){ 32.0, 32.0 });

		phx_update(delta);
		ent_update(delta);
		ent_render();
		

		SDL_GetWindowSize(GLOBAL.window, &w, &h);

		gfx_make_framebuffers(w, h);
		gfx_clear_framebuffers();
	
		gfx_setup_draw_framebuffers();
		gfx_scene_draw();
		gfx_set_camera((vec2){0.0, 0.0}, (vec2){ 1.0, 1.0 });

		gfx_end_draw_framebuffers();
		gfx_render_present();

		ui_cleanup();
		ui_draw();
		ui_order();
		SDL_GL_SwapWindow(GLOBAL.window);
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				goto end_loop;
			case SDL_MOUSEMOTION:
				ui_mouse_motion(event.motion.x, event.motion.y);
				break;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
				switch(event.button.button) {
				case SDL_BUTTON_LEFT: ui_mouse_button(UI_MOUSE_LEFT, event.type == SDL_MOUSEBUTTONDOWN); break;
				case SDL_BUTTON_RIGHT: ui_mouse_button(UI_MOUSE_RIGHT, event.type == SDL_MOUSEBUTTONDOWN); break;
				case SDL_BUTTON_MIDDLE: ui_mouse_button(UI_MOUSE_MIDDLE, event.type == SDL_MOUSEBUTTONDOWN); break;
				}
				break;
			}
		}
	}

end_loop:
	ui_terminate();
	phx_end();
	ent_end();
	gfx_end();
	gfx_scene_cleanup();

	SDL_DestroyRenderer(GLOBAL.renderer);
	SDL_DestroyWindow(GLOBAL.window);
	SDL_Quit();
	return 0;
}
