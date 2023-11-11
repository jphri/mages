#include <stdio.h>
#include <stdbool.h>
#include <glad/gles2.h>
#include <SDL2/SDL.h>

#include "global.h"
#include "vecmath.h"
#include "physics.h"
#include "entity.h"
#include "graphics.h"
#include "util.h"
#include "map.h"
#include "editor.h"

Global GLOBAL;
EditorGlobal editor;

static GLADapiproc load_proc(const char *name) 
{
	GLADapiproc proc;

	*(void**)(&proc)= SDL_GL_GetProcAddress(name);
	return proc;
}

static State state_vtable[] = {
	[EDITOR_EDIT_MAP] = {
		.render = edit_render,
		.wheel = edit_wheel,
		.keyboard = edit_keyboard,
		.mouse_button = edit_mouse_button,
		.mouse_motion = edit_mouse_motion
	},
	[EDITOR_SELECT_TILE] = {
		.render       = select_tile_render,
		.wheel        = select_tile_wheel,
		.keyboard     = select_tile_keyboard,
		.mouse_button = select_tile_mouse_button,
		.mouse_motion = select_tile_mouse_motion
	},
	[EDITOR_EDIT_COLLISION] = {
		.render       = collision_render,
		.wheel        = collision_wheel,
		.keyboard     = collision_keyboard,
		.mouse_button = collision_mouse_button,
		.mouse_motion = collision_mouse_motion
	}
};

int
main(int argc, char *argv[])
{
	if(argc == 3) {
		editor.map = map_alloc(atoi(argv[1]), atoi(argv[2]));
	}
	if(argc == 2) {
		load_map(argv[1]);
	}

	editor.map_atlas = SPRITE_TERRAIN;
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
	gladLoadGLES2(load_proc);

	gfx_init();
	while(true) {
		int w, h;
		SDL_Event event;
		SDL_GetWindowSize(GLOBAL.window, &w, &h);

		gfx_make_framebuffers(w, h);
		glClear(GL_COLOR_BUFFER_BIT);
		
		if(state_vtable[editor.editor_state].render)
			state_vtable[editor.editor_state].render();
		
		SDL_GL_SwapWindow(GLOBAL.window);
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				goto end_loop;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
				if(state_vtable[editor.editor_state].mouse_button)
					state_vtable[editor.editor_state].mouse_button(&event);
				break;
			case SDL_MOUSEWHEEL:
				if(state_vtable[editor.editor_state].wheel)
					state_vtable[editor.editor_state].wheel(&event);
				break;
			case SDL_MOUSEMOTION:
				if(state_vtable[editor.editor_state].mouse_motion)
					state_vtable[editor.editor_state].mouse_motion(&event);
				break;
			case SDL_KEYDOWN:
				if(state_vtable[editor.editor_state].keyboard)
					state_vtable[editor.editor_state].keyboard(&event);
				break;
			case SDL_KEYUP:
				if(state_vtable[editor.editor_state].keyboard)
					state_vtable[editor.editor_state].keyboard(&event);
			}
		}
	}

end_loop:
	gfx_end();

	SDL_DestroyRenderer(GLOBAL.renderer);
	SDL_DestroyWindow(GLOBAL.window);
	SDL_Quit();
	return 0;
}

void
export_map(const char *map_file) 
{
	size_t map_data_size;
	char *map_data = map_export(editor.map, &map_data_size);
	FILE *fp = fopen(map_file, "w");
	if(!fp)
		goto error_open;
	
	int f = fwrite(map_data, 1, map_data_size, fp);
	if(f < (int)(map_data_size))
		printf("Error saving file: %s\n", map_file);
	else
		printf("File saved at %s\n", map_file);
	
	fclose(fp);
	
error_open:
	free(map_data);
}

void
load_map(const char *map_file)
{
	editor.map = map_load(map_file);
	if(!editor.map)
		die("failed loading file %s\n", map_file);
}

