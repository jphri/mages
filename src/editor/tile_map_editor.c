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
#include "../ui.h"
#include "editor.h"

EditorGlobal editor;
static State state_vtable[] = {
	[EDITOR_EDIT_MAP] = {
		.render = edit_render,
		.wheel = edit_wheel,
		.keyboard = edit_keyboard,
		.mouse_button = edit_mouse_button,
		.mouse_motion = edit_mouse_motion,
		.enter = edit_enter,
		.exit = edit_exit,
		.init = edit_init,
		.terminate = edit_terminate,
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

static void select_mode_cbk(UIObject obj, void *ptr)
{
	(void)obj;

	EditorState mode = (State*)ptr - state_vtable;
	editor_change_state(mode);
}

void
GAME_STATE_LEVEL_EDIT_init(void)
{
	UIObject layout = ui_layout_new();
	ui_layout_set_order(layout, UI_LAYOUT_HORIZONTAL);

	#define BUTTON_MODE(MODE, ICON_X, ICON_Y) \
	{ \
		UIObject btn = ui_button_new(); \
		UIObject img = ui_image_new();  \
		ui_image_set_stamp(img, &(TextureStamp){   \
			.texture = TEXTURE_UI, \
			.position = { (ICON_X) / 256.0, (ICON_Y) / 256.0 }, \
			.size     = { 8.0 / 256.0, 8.0 / 256.0 } \
		}); \
		ui_image_set_keep_aspect(img, true); \
		ui_button_set_label(btn, img); \
		ui_button_set_callback(btn, &state_vtable[MODE], select_mode_cbk); \
		ui_layout_append(layout, btn); \
	}

	BUTTON_MODE(EDITOR_EDIT_MAP, 2 * 8, 0 * 8);
	BUTTON_MODE(EDITOR_EDIT_COLLISION, 3 * 8, 0 * 8);
	BUTTON_MODE(EDITOR_SELECT_TILE, 4 * 8, 0 * 8);

	#undef BUTTON_MODE

	UIObject window = ui_window_new();
	ui_window_set_size(window, (vec2){ 120, 30 });
	ui_window_set_position(window, (vec2){ 120 + 0, 30 + 0 });
	ui_window_set_border(window, (vec2){ 2, 2 });
	ui_window_set_decorated(window, false);
	ui_window_set_child(window, layout);
	
	ui_child_append(ui_root(), window);

	editor.map_atlas = SPRITE_TERRAIN;
	if(editor.map == NULL)
		editor.map = map_load("maps/test_map_2.map");

	for(int i = 0; i < LAST_EDITOR_STATE; i++) {
		if(state_vtable[i].init)
			state_vtable[i].init();
	}

	if(state_vtable[editor.editor_state].enter) {
		state_vtable[editor.editor_state].enter();
	}
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
	if(ui_is_active())
		return;
	if(state_vtable[editor.editor_state].mouse_button)
		state_vtable[editor.editor_state].mouse_button(event);
}

void
GAME_STATE_LEVEL_EDIT_mouse_wheel(SDL_Event *event)
{
	if(ui_is_active())
		return;
	if(state_vtable[editor.editor_state].wheel)
		state_vtable[editor.editor_state].wheel(event);
}

void
GAME_STATE_LEVEL_EDIT_mouse_move(SDL_Event *event) 
{
	if(ui_is_active())
		return;
	if(state_vtable[editor.editor_state].mouse_motion)
		state_vtable[editor.editor_state].mouse_motion(event);
}

void
GAME_STATE_LEVEL_EDIT_keyboard(SDL_Event *event)
{
	if(ui_is_active())
		return;

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
	for(int i = 0; i < LAST_EDITOR_STATE; i++) {
		if(state_vtable[i].terminate)
			state_vtable[i].terminate();
	}
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

void
editor_change_state(EditorState state)
{
	if(state_vtable[editor.editor_state].exit) {
		state_vtable[editor.editor_state].exit();
	}

	if(state_vtable[state].enter) {
		state_vtable[state].enter();
	}
	editor.editor_state = state;
	
}
