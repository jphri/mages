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

static void hello_callback(void *ptr)
{
	(void)ptr;
	printf("HELLO!\n");
}

static void hello2_callback(void *ptr)
{
	(void)ptr;
	printf("HELLO! 2\n");
}

static void hello3_callback(void *ptr)
{
	(void)ptr;
	hello_count++;

	arrbuf_clear(&label_ptr);
	arrbuf_printf(&label_ptr, "Hello count: %d\n", hello_count);
	ui_label_set_text(label, label_ptr.data);
}

int
main()
{
	(void)hello_callback;
	(void)hello2_callback;
	(void)hello3_callback;

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
	ui_obj_set_size(window, (vec2){ 90, 90 });
	ui_obj_set_position(window, (vec2){ 300, 100 });
	ui_window_set_bg(window, (vec4){ 0.2, 0.2, 0.2, 1.0 });
	ui_window_set_border(window, (vec4){ 0.6, 0.6, 0.6, 1.0 });
	ui_window_set_border_size(window, (vec2){ 3, 3 });

	UIObject layout3 = ui_new_object(window, UI_LAYOUT);
	ui_obj_set_size(layout3, (vec2){ 70, 70 });
	ui_obj_set_position(layout3, (vec2) { 90, 90 });
	ui_layout_set_order(layout3, UI_LAYOUT_VERTICAL);

	UIObject layout2 = ui_new_object(layout3, UI_LAYOUT);
	ui_obj_set_position(layout2, (vec2){ 74, 42 });
	ui_obj_set_size(layout2, (vec2){ 64, 32 });
	ui_layout_set_order(layout2, UI_LAYOUT_HORIZONTAL) ;

	UIObject layout = ui_new_object(layout2, UI_LAYOUT);
	ui_obj_set_position(layout, (vec2){ 42, 42 });
	ui_obj_set_size(layout, (vec2){ 32, 32 });
	ui_layout_set_order(layout, UI_LAYOUT_VERTICAL) ;

	UIObject dummy = ui_new_object(layout, UI_BUTTON);
	ui_button_set_label(dummy, "hello");
	ui_button_set_userptr(dummy, NULL);
	ui_button_set_callback(dummy, hello_callback);
	ui_button_set_label_border(dummy, (vec2){ 5.0, 5.0 });

	UIObject dummy_2 = ui_new_object(layout, UI_BUTTON);
	ui_button_set_label(dummy_2, "hello 2");
	ui_button_set_userptr(dummy_2, NULL);
	ui_button_set_callback(dummy_2, hello2_callback);
	ui_button_set_label_border(dummy_2, (vec2){ 5.0, 5.0 });

	UIObject dummy_3 = ui_new_object(layout2, UI_BUTTON);
	ui_button_set_label(dummy_3, "hello 3");
	ui_button_set_userptr(dummy_3, NULL);
	ui_button_set_callback(dummy_3, hello3_callback);
	ui_button_set_label_border(dummy_3, (vec2){ 5.0, 5.0 });

	label = ui_new_object(layout3, UI_LABEL);
	ui_label_set_color(label, (vec4){ 1.0, 1.0, 0.0, 1.0 });
	ui_label_set_border(label, (vec2){ 10.0, 10.0 });
	ui_label_set_text(label, "Hello");

	map = map_load("maps/test_map_2.map");
	map_set_gfx_scene(map);
	map_set_phx_scene(map);
	map_free(map);

	EntityID player_id = ent_player_new((vec2){ 15.0, 15.0 });
	Uint64 prev_time = SDL_GetPerformanceCounter();

	(void)player_id;
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
	
		gfx_setup_draw_framebuffers();
		gfx_scene_draw();


		gfx_set_camera((vec2){0.0, 0.0}, (vec2){ 1.0, 1.0 });
		gfx_draw_begin(NULL);
		gfx_draw_sprite(&(Sprite) {
			.type = TEXTURE_UI,
			.color = { 1.0, 0.0, 1.0, 1.0 },
			.position = { 30, 30 },
			.clip_region = { 0, 0, 1000, 1000 }, 
			.half_size = { 10, 10 }
		});
		gfx_draw_sprite(&(Sprite) {
			.type = TEXTURE_UI,
			.color = { 1.0, 0.0, 1.0, 1.0 },
			.position = { 150, SDL_GetTicks() / 10.0 },
			.clip_region = { 0, 0, 1000, 1000 }, 
			.half_size = { 10, 10 }
		});
		gfx_draw_line(TEXTURE_UI, (vec2){ 30, 30 }, (vec2){ 150, SDL_GetTicks() / 10.0 }, 10.0, (vec4){ 1.0, 1.0, 1.0, 1.0 }, (vec4){ 0, 0, 1000, 1000 });
		gfx_draw_end();

		gfx_end_draw_framebuffers();
		gfx_render_present();
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
