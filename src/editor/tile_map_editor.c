#include <stdio.h>
#include <stdbool.h>
#include <glad/gles2.h>

#include <SDL.h>

#include "../game_state.h"
#include "../global.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"
#include "../graphics.h"
#include "../util.h"
#include "../map.h"
#include "editor.h"

EditorGlobal editor;
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

void
GAME_STATE_LEVEL_EDIT_init(void)
{
	editor.map_atlas = SPRITE_TERRAIN;
	if(editor.map == NULL)
		editor.map = map_load("maps/test_map_2.map");
}

void
GAME_STATE_LEVEL_EDIT_render(void)
{
	if(state_vtable[editor.editor_state].render)
		state_vtable[editor.editor_state].render();
}

void
GAME_STATE_LEVEL_EDIT_mouse_button(SDL_Event *event)
{
	if(state_vtable[editor.editor_state].mouse_button)
		state_vtable[editor.editor_state].mouse_button(event);
}

void
GAME_STATE_LEVEL_EDIT_mouse_wheel(SDL_Event *event)
{
	if(state_vtable[editor.editor_state].wheel)
		state_vtable[editor.editor_state].wheel(event);
}

void
GAME_STATE_LEVEL_EDIT_mouse_move(SDL_Event *event) 
{
	if(state_vtable[editor.editor_state].mouse_motion)
		state_vtable[editor.editor_state].mouse_motion(event);
}

void
GAME_STATE_LEVEL_EDIT_keyboard(SDL_Event *event)
{
	if(state_vtable[editor.editor_state].keyboard)
		state_vtable[editor.editor_state].keyboard(event);
	
	if(event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_K)
		gstate_set(GAME_STATE_LEVEL);
}


void 
GAME_STATE_LEVEL_EDIT_update(float delta)
{
	(void)delta;
}

void 
GAME_STATE_LEVEL_EDIT_end(void) 
{
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

