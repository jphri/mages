#include <stdio.h>
#include <stdbool.h>
#include <glad/gles2.h>

#include <SDL.h>

#include "SDL_keyboard.h"
#include "SDL_timer.h"
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
#include "audio.h"
#include "util.h"
#include "events.h"

static void *cache_line_allocate(size_t size, void *user);
static void  cache_line_deallocate(void *ptr, void *user);
static GLADapiproc load_proc(const char *name);

Global GLOBAL;
static float fps_time;
static int fps;
static Uint64 rendering_time;
static GameStateVTable *current_state, *next_state;

int
main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	Uint64 prev_time;

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		printf("SDL_Init() failed: %s\n", SDL_GetError());
		return -1;
	}

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
		printf("SDL_GL_CreateContext() with OpenGL ES 3.1 failed: %s\n", SDL_GetError());
		printf("Trying OpenGL 3.3\n");

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		GLOBAL.glctx = SDL_GL_CreateContext(GLOBAL.window);

		if(!GLOBAL.glctx) {
			printf("SDL_GL_CreateContext() with OpenGL 3.3 failed: %s\n", SDL_GetError());
			return 0;
		}
		
	}
	SDL_GL_SetSwapInterval(1);
	SDL_GL_MakeCurrent(GLOBAL.window, GLOBAL.glctx);
	gladLoadGLES2(load_proc);
	printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

	event_init();
	gfx_init();
	gfx_scene_setup();
	phx_init();
	ent_init();
	ui_init();
	audio_init();

	prev_time = SDL_GetPerformanceCounter();

	start_game_level_edit();
	current_state = next_state;
	next_state = NULL;

	if(current_state->init)
		current_state->init();

	while(true) {
		SDL_Event event;
		int w, h;

		event_cleanup();
		ui_cleanup();
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				goto end_loop;
			case SDL_MOUSEMOTION:
				ui_mouse_motion(event.motion.x, event.motion.y);
				if(current_state->mouse_move)
					current_state->mouse_move(&event);
				break;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
				switch(event.button.button) {
				case SDL_BUTTON_LEFT: ui_mouse_button(UI_MOUSE_LEFT, event.type == SDL_MOUSEBUTTONDOWN); break;
				case SDL_BUTTON_RIGHT: ui_mouse_button(UI_MOUSE_RIGHT, event.type == SDL_MOUSEBUTTONDOWN); break;
				case SDL_BUTTON_MIDDLE: ui_mouse_button(UI_MOUSE_MIDDLE, event.type == SDL_MOUSEBUTTONDOWN); break;
				}
				if(current_state->mouse_button)
					current_state->mouse_button(&event);
				break;
			case SDL_KEYDOWN:
				if(event.type == SDL_KEYDOWN)
					switch(event.key.keysym.scancode) {
						case SDL_SCANCODE_LEFT:      ui_key(UI_KEY_LEFT);      break;
						case SDL_SCANCODE_RIGHT:     ui_key(UI_KEY_RIGHT);     break;
						case SDL_SCANCODE_UP:        ui_key(UI_KEY_UP);        break;
						case SDL_SCANCODE_DOWN:      ui_key(UI_KEY_DOWN);      break;
						case SDL_SCANCODE_BACKSPACE: ui_key(UI_KEY_BACKSPACE); break;
						case SDL_SCANCODE_RETURN:    ui_key(UI_KEY_ENTER);     break;
						default: break;
					}
				/* fallthrough */
			case SDL_KEYUP:
				if(current_state->keyboard)
					current_state->keyboard(&event);
				break;
			case SDL_MOUSEWHEEL:
				if(current_state->mouse_wheel)
					current_state->mouse_wheel(&event);
				break;
			case SDL_TEXTINPUT:
				ui_text(event.text.text, strlen(event.text.text));
				break;
			}
		}

		Uint64 curr_time = SDL_GetPerformanceCounter();
		float delta = (float)(curr_time - prev_time) / SDL_GetPerformanceFrequency();
		prev_time = curr_time;

		if(current_state->update)
			current_state->update(delta);

		SDL_GetWindowSize(GLOBAL.window, &w, &h);
		glViewport(0, 0, w, h);

		Uint64 begin_render_time = SDL_GetPerformanceCounter();
		gfx_make_framebuffers(w, h);
		current_state->render(w, h);
		Uint64 end_render_time = SDL_GetPerformanceCounter();

		SDL_GL_SwapWindow(GLOBAL.window);
		if(next_state) {
			current_state->end();
			current_state = next_state;
			current_state->init();
			next_state = NULL;
		}

		rendering_time += end_render_time - begin_render_time;

		fps++;
		fps_time += delta;
		if(fps_time > 1.0) {
			double rend_time = rendering_time / (double)SDL_GetPerformanceFrequency();
				   rend_time /= fps;
			printf("FPS: %d | Avg rend time: %f ms (%0.2f estimated FPS) | sprites rendered: %d | draw count: %d | sprites per draw call: %0.2f \n", fps, rend_time * 1000, 1.0 / rend_time, gfx_debug_sprites_rendered(), gfx_debug_draw_count(), (double)gfx_debug_sprites_rendered() / gfx_debug_draw_count());
			fps_time = 0;
			fps = 0;

			gfx_debug_reset();

			rendering_time = 0;
		}
	}

end_loop:
	current_state->end();

	phx_end();
	ent_end();
	audio_end();
	event_terminate();
	ui_terminate();
	gfx_terminate();

	SDL_DestroyRenderer(GLOBAL.renderer);
	SDL_DestroyWindow(GLOBAL.window);
	SDL_Quit();

	return 0;
}

void
game_change_state_vtable(GameStateVTable *new_vtable)
{
	next_state = new_vtable;
}

Allocator
cache_aligned_allocator(void)
{
	return (Allocator) {
		.userptr = NULL,
		.allocate = cache_line_allocate,
		.deallocate = cache_line_deallocate
	};
}

void
enable_text_input(void)
{
	SDL_StartTextInput();
}

void
disable_text_input(void)
{
	SDL_StopTextInput();
}

static GLADapiproc load_proc(const char *name) 
{
	GLADapiproc proc;

	*(void**)(&proc)= SDL_GL_GetProcAddress(name);
	return proc;
}

#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))

static void *cache_line_allocate(size_t size, void *user)
{
	(void)user;
	void *ptr;
	size = ((size + SDL_GetCPUCacheLineSize() - 1) / SDL_GetCPUCacheLineSize()) * SDL_GetCPUCacheLineSize();

	if(posix_memalign(&ptr, SDL_GetCPUCacheLineSize(), size) > 0)
		return NULL;
	else
		return ptr;
}

static void cache_line_deallocate(void *ptr, void *user)
{
	(void)user;
	free(ptr);
}

#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
static void *cache_line_allocate(size_t size, void *user)
{
	(void)user;
	size = ((size + SDL_GetCPUCacheLineSize() - 1) / SDL_GetCPUCacheLineSize()) * SDL_GetCPUCacheLineSize();
	return _aligned_malloc(size, SDL_GetCPUCacheLineSize());
}

static void cache_line_deallocate(void *ptr, void *user)
{
	(void)user;
	_aligned_free(ptr);
}


#else
#error "Only WINDOWS or UNIX."
#endif
