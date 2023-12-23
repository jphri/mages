#include <stdio.h>
#include <stdbool.h>
#include <glad/gles2.h>

#include <SDL2/SDL.h>

#include "game_objects.h"
#include "global.h"
#include "game_state.h"
#include "ui.h"
#include "global.h"
#include "vecmath.h"
#include "physics.h"
#include "entity.h"
#include "graphics.h"
#include "util.h"
#include "map.h"

typedef struct {
	void (*init)(void);
	void (*end)(void);
	void (*update)(float delta); 
	void (*render)(void);
	void (*keyboard)(SDL_Event *event);
	void (*mouse_move)(SDL_Event *event);
	void (*mouse_button)(SDL_Event *event);
	void (*mouse_wheel)(SDL_Event *event);
} GameStateVTable;

static GLADapiproc load_proc(const char *name) 
{
	GLADapiproc proc;

	*(void**)(&proc)= SDL_GL_GetProcAddress(name);
	return proc;
}

static GameStateVTable state_vtable[] = {
	#define GAME_ST_DEF(STATE_NAME) \
		[STATE_NAME] = {\
			.init = STATE_NAME##_init,\
			.end = STATE_NAME##_end,\
			.update = STATE_NAME##_update,\
			.render = STATE_NAME##_render,\
			.keyboard = STATE_NAME##_keyboard,\
			.mouse_move = STATE_NAME##_mouse_move,\
			.mouse_button = STATE_NAME##_mouse_button,\
			.mouse_wheel = STATE_NAME##_mouse_wheel, \
		},

	GAME_STATE_LIST
	
	#undef GAME_ST_DEF
};

static GameState current_state, next_state;
static bool need_change;

Global GLOBAL;

void
gstate_set(GameState state)
{
	need_change = true;
	next_state = state;
}

int
main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	Uint64 prev_time;

	GLOBAL.window = SDL_CreateWindow("hello",
							  SDL_WINDOWPOS_UNDEFINED,
							  SDL_WINDOWPOS_UNDEFINED,
							  800, 600,
							  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	GLOBAL.renderer = SDL_CreateRenderer(GLOBAL.window, -1, SDL_RENDERER_ACCELERATED);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	GLOBAL.glctx = SDL_GL_CreateContext(GLOBAL.window);
	if(!GLOBAL.glctx) {
		printf("SDL_GL_CreateContext() failed\n");
		return 0;
	}
	SDL_GL_SetSwapInterval(1);
	SDL_GL_MakeCurrent(GLOBAL.window, GLOBAL.glctx);
	gladLoadGLES2(load_proc);
	printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

	obj_init((GameObjectRegistrar){
		[GAME_OBJECT_TYPE_BODY]      = phx_object_descr(),
		[GAME_OBJECT_TYPE_ENTITY]    = ent_object_descr(),
		[GAME_OBJECT_TYPE_GFX_SCENE] = gfx_scene_object_descr(),
	});

	gfx_init();
	gfx_scene_setup();
	phx_init();
	ent_init();
	ui_init();

	state_vtable[current_state].init();

	prev_time = SDL_GetPerformanceCounter();
	while(true) {
		SDL_Event event;
		int w, h;

		obj_clean();

		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				goto end_loop;
			case SDL_MOUSEMOTION:
				ui_mouse_motion(event.motion.x, event.motion.y);
				if(!ui_is_active())
					state_vtable[current_state].mouse_move(&event);
				break;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
				switch(event.button.button) {
				case SDL_BUTTON_LEFT: ui_mouse_button(UI_MOUSE_LEFT, event.type == SDL_MOUSEBUTTONDOWN); break;
				case SDL_BUTTON_RIGHT: ui_mouse_button(UI_MOUSE_RIGHT, event.type == SDL_MOUSEBUTTONDOWN); break;
				case SDL_BUTTON_MIDDLE: ui_mouse_button(UI_MOUSE_MIDDLE, event.type == SDL_MOUSEBUTTONDOWN); break;
				}
				if(!ui_is_active())
					state_vtable[current_state].mouse_button(&event);
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if(!ui_is_active())
					state_vtable[current_state].keyboard(&event);
				break;
			case SDL_MOUSEWHEEL:
				if(!ui_is_active())
					state_vtable[current_state].mouse_wheel(&event);
			}
		}

		Uint64 curr_time = SDL_GetPerformanceCounter();
		float delta = (float)(curr_time - prev_time) / SDL_GetPerformanceFrequency();
		prev_time = curr_time;

		state_vtable[current_state].update(delta);
		gfx_scene_update(delta);
		phx_update(delta);
		ent_update(delta);

		SDL_GetWindowSize(GLOBAL.window, &w, &h);

		gfx_make_framebuffers(w, h);
		//gfx_setup_draw_framebuffers();
		gfx_clear();
		

		gfx_camera_set_enabled(true);
		gfx_scene_draw();
		//gfx_end_draw_framebuffers();
		//gfx_render_present();

		ent_render();
		state_vtable[current_state].render();

		gfx_camera_set_enabled(false);
		ui_cleanup();
		ui_order();
		ui_draw();

		SDL_GL_SwapWindow(GLOBAL.window);
		if(need_change) {
			state_vtable[current_state].end();
			current_state = next_state;
			state_vtable[current_state].init();

			need_change = false;
		}
	}

end_loop:
	ui_terminate();
	phx_end();
	ent_end();
	gfx_end();
	obj_terminate();

	SDL_DestroyRenderer(GLOBAL.renderer);
	SDL_DestroyWindow(GLOBAL.window);
	SDL_Quit();

	return 0;
}
