#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
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
	[EDITOR_EDIT_COLLISION] = {
		.render       = collision_render,
		.wheel        = collision_wheel,
		.keyboard     = collision_keyboard,
		.mouse_button = collision_mouse_button,
		.mouse_motion = collision_mouse_motion,
		.enter = collision_enter,
		.exit = collision_exit,
		.init = collision_init,
		.terminate = collision_terminate,
	}
};

enum {
	CONTEXT_MENU = 0x01,
	GENERAL_MENU = 0x02
};

static int menu_shown;

static UIObject new_window, new_width, new_height;
static UIObject load_window, load_path;
static UIObject save_window, save_path;
static UIObject extra_window;
static UIObject cursor_position;

static ArrayBuffer cursor_pos_str;

static void open_cbk(UIObject obj, void (*userptr));
static void cancel_cbk(UIObject btn, void *userptr);
static void newbtn_new_cbk(UIObject btn, void *userptr);
static void loadbtn_load_cbk(UIObject btn, void *userptr);
static void save_btn_cbk(UIObject btn, void *userptr);

static void context_btn_cbk(UIObject btn, void *userptr);
static void general_btn_cbk(UIObject btn, void *userptr);

static void process_open_menu(void);
static void update_cursor_pos(vec2 v);

static void select_mode_cbk(UIObject obj, void *ptr)
{
	(void)obj;

	EditorState mode = (State*)ptr - state_vtable;
	editor_change_state(mode);
}

void
GAME_STATE_LEVEL_EDIT_init(void)
{
	arrbuf_init(&cursor_pos_str);

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

	#undef BUTTON_MODE

	UIObject window = ui_window_new();
	ui_window_set_size(window, (vec2){ 80, 30 });
	ui_window_set_position(window, UI_ORIGIN_TOP_LEFT, (vec2){ 80, 30 });
	ui_window_set_border(window, (vec2){ 2, 2 });
	ui_window_set_decorated(window, false);
	ui_window_append_child(window, layout);
	ui_child_append(ui_root(), window);

	UIObject cursor_position_window = ui_window_new();
	ui_window_set_size(cursor_position_window, (vec2){ 50, 15 });
	ui_window_set_position(cursor_position_window, UI_ORIGIN_TOP_LEFT, (vec2){ 50, 60 + 15 });
	ui_window_set_decorated(cursor_position_window, false);
	{
		cursor_position = ui_label_new();
		ui_label_set_color(cursor_position, (vec4){ 1.0, 1.0, 0.0, 1.0 });
		ui_label_set_text(cursor_position, "0, 0");
		ui_label_set_alignment(cursor_position, UI_LABEL_ALIGN_CENTER);
		ui_window_append_child(cursor_position_window, cursor_position);
	}
	ui_child_append(ui_root(), cursor_position_window);

	UIObject file_buttons_window = ui_window_new();
	ui_window_set_decorated(file_buttons_window, false);
	ui_window_set_size(file_buttons_window, (vec2){ 40, 40 });
	ui_window_set_position(file_buttons_window, UI_ORIGIN_TOP_RIGHT, (vec2){ -40, 40 });
	{
		UIObject layout = ui_layout_new();
		ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
		{
			UIObject save_btn = ui_button_new();
			ui_button_set_callback(save_btn, &save_window, open_cbk);
			
			{
				UIObject save_label = ui_label_new();
				ui_label_set_text(save_label, "Save");
				ui_label_set_color(save_label, (vec4){ 1.0, 1.0, 0.0, 1.0 });
				ui_button_set_label(save_btn, save_label);
			}
			ui_layout_append(layout, save_btn);

			UIObject load_btn = ui_button_new();
			ui_button_set_callback(load_btn, &load_window, open_cbk);
			
			{
				UIObject load_label = ui_label_new();
				ui_label_set_text(load_label, "Load");
				ui_label_set_color(load_label, (vec4){ 1.0, 1.0, 0.0, 1.0 });
				ui_button_set_label(load_btn, load_label);
			}
			ui_layout_append(layout, load_btn);

			UIObject new_btn = ui_button_new();
			ui_button_set_callback(new_btn, &new_window, open_cbk);
			{
				UIObject label = ui_label_new();
				ui_label_set_text(label, "New");
				ui_label_set_color(label, (vec4){ 1.0, 1.0, 0.0, 1.0 });
				ui_button_set_label(new_btn, label);
			}
			ui_layout_append(layout, new_btn);
		}

		ui_window_append_child(file_buttons_window, layout);
	}
	ui_child_append(ui_root(), file_buttons_window);

	new_window = ui_window_new();
	ui_window_set_size(new_window, (vec2){ 120, 90 });
	ui_window_set_position(new_window, UI_ORIGIN_CENTER, (vec2){ 0.0, 0.0 });
	ui_window_set_title(new_window, "New");
	ui_window_set_decorated(new_window, true);
	{
		UIObject layout = ui_layout_new();
		ui_layout_set_border(layout, 30, 30, 20, 20);
		ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
		ui_layout_set_fixed_size(layout, 20.0);
		{
			UIObject subv, lbl;
			
			#define SUBV(NAME_FIELD)\
				subv = ui_layout_new(); \
				ui_layout_set_order(subv, UI_LAYOUT_HORIZONTAL); \
				ui_layout_set_border(subv, 5, 5, 5, 5); \
				lbl = ui_label_new(); \
				ui_label_set_alignment(lbl, UI_LABEL_ALIGN_CENTER);\
				ui_label_set_color(lbl, (vec4){ 1.0, 1.0, 0.0, 1.0 });\
				ui_label_set_text(lbl, NAME_FIELD);\
				ui_child_append(subv, lbl); 

			#define END_SUBV ui_layout_append(layout, subv)
			
			SUBV("Width: ") {
				new_width = ui_text_input_new();
				ui_text_input_set_filter(new_width, isdigit);
				ui_child_append(subv, new_width);
			} END_SUBV;

			SUBV("Height: ") {
				new_height = ui_text_input_new();
				ui_text_input_set_filter(new_height, isdigit);
				ui_child_append(subv, new_height);
			} END_SUBV;

			subv = ui_layout_new();
			ui_layout_set_order(subv, UI_LAYOUT_HORIZONTAL); \
			ui_layout_set_border(subv, 5, 5, 5, 5);
			{
				UIObject new = ui_button_new();
				ui_button_set_callback(new, NULL, newbtn_new_cbk);
				{
					UIObject label = ui_label_new();
					ui_label_set_text(label, "New");
					ui_label_set_alignment(label, UI_LABEL_ALIGN_CENTER);
					ui_button_set_label(new, label);
				}
				ui_child_append(subv, new);

				UIObject cancel = ui_button_new();
				ui_button_set_callback(cancel, &new_window, cancel_cbk);
				
				{
					UIObject label = ui_label_new();
					ui_label_set_text(label, "Cancel");
					ui_label_set_alignment(label, UI_LABEL_ALIGN_CENTER);
					ui_button_set_label(cancel, label);
				}
				ui_child_append(subv, cancel);
			}
			ui_child_append(layout, subv);
		}

		ui_window_append_child(new_window, layout);
	}

	load_window = ui_window_new();
	ui_window_set_size(load_window, (vec2){ 120, 90 });
	ui_window_set_position(load_window, UI_ORIGIN_CENTER, (vec2){ 0.0, 0.0 });
	ui_window_set_title(load_window, "New");
	ui_window_set_decorated(load_window, true);
	{
		UIObject layout = ui_layout_new();
		ui_layout_set_border(layout, 30, 30, 20, 20);
		ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
		ui_layout_set_fixed_size(layout, 20.0);
		{
			UIObject subv, lbl;
			
			#define SUBV(NAME_FIELD)\
				subv = ui_layout_new(); \
				ui_layout_set_order(subv, UI_LAYOUT_HORIZONTAL); \
				ui_layout_set_border(subv, 5, 5, 5, 5); \
				lbl = ui_label_new(); \
				ui_label_set_alignment(lbl, UI_LABEL_ALIGN_CENTER);\
				ui_label_set_color(lbl, (vec4){ 1.0, 1.0, 0.0, 1.0 });\
				ui_label_set_text(lbl, NAME_FIELD);\
				ui_child_append(subv, lbl); 

			#define END_SUBV ui_layout_append(layout, subv)
			
			SUBV("Path: ") {
				load_path = ui_text_input_new();
				ui_child_append(subv, load_path);
			} END_SUBV;

			subv = ui_layout_new();
			ui_layout_set_order(subv, UI_LAYOUT_HORIZONTAL); \
			ui_layout_set_border(subv, 5, 5, 5, 5);
			{
				UIObject load = ui_button_new();
				ui_button_set_callback(load, NULL, loadbtn_load_cbk);
				{
					UIObject label = ui_label_new();
					ui_label_set_text(label, "Load");
					ui_label_set_alignment(label, UI_LABEL_ALIGN_CENTER);
					ui_button_set_label(load, label);
				}
				ui_child_append(subv, load);

				UIObject cancel = ui_button_new();
				ui_button_set_callback(cancel, &load_window, cancel_cbk);
				
				{
					UIObject label = ui_label_new();
					ui_label_set_text(label, "Cancel");
					ui_label_set_alignment(label, UI_LABEL_ALIGN_CENTER);
					ui_button_set_label(cancel, label);
				}
				ui_child_append(subv, cancel);
			}
			ui_child_append(layout, subv);
		}

		ui_window_append_child(load_window, layout);
	}

	save_window = ui_window_new();
	ui_window_set_size(save_window, (vec2){ 120, 90 });
	ui_window_set_position(save_window, UI_ORIGIN_CENTER, (vec2){ 0.0, 0.0 });
	ui_window_set_title(save_window, "New");
	ui_window_set_decorated(save_window, true);
	{
		UIObject layout = ui_layout_new();
		ui_layout_set_border(layout, 30, 30, 20, 20);
		ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
		ui_layout_set_fixed_size(layout, 20.0);
		{
			UIObject subv, lbl;
			
			#define SUBV(NAME_FIELD)\
				subv = ui_layout_new(); \
				ui_layout_set_order(subv, UI_LAYOUT_HORIZONTAL); \
				ui_layout_set_border(subv, 5, 5, 5, 5); \
				lbl = ui_label_new(); \
				ui_label_set_alignment(lbl, UI_LABEL_ALIGN_CENTER);\
				ui_label_set_color(lbl, (vec4){ 1.0, 1.0, 0.0, 1.0 });\
				ui_label_set_text(lbl, NAME_FIELD);\
				ui_child_append(subv, lbl); 

			#define END_SUBV ui_layout_append(layout, subv)
			
			SUBV("Path: ") {
				save_path = ui_text_input_new();
				ui_child_append(subv, save_path);
			} END_SUBV;

			subv = ui_layout_new();
			ui_layout_set_order(subv, UI_LAYOUT_HORIZONTAL); \
			ui_layout_set_border(subv, 5, 5, 5, 5);
			{
				UIObject save = ui_button_new();
				ui_button_set_callback(save, NULL, save_btn_cbk);
				{
					UIObject label = ui_label_new();
					ui_label_set_text(label, "Save");
					ui_label_set_alignment(label, UI_LABEL_ALIGN_CENTER);
					ui_button_set_label(save, label);
				}
				ui_child_append(subv, save);

				UIObject cancel = ui_button_new();
				ui_button_set_callback(cancel, &save_window, cancel_cbk);
				{
					UIObject label = ui_label_new();
					ui_label_set_text(label, "Cancel");
					ui_label_set_alignment(label, UI_LABEL_ALIGN_CENTER);
					ui_button_set_label(cancel, label);
				}
				ui_child_append(subv, cancel);
			}
			ui_child_append(layout, subv);
		}

		ui_window_append_child(save_window, layout);
	}

	extra_window = ui_window_new();
	ui_window_set_decorated(extra_window, false);
	ui_window_set_size(extra_window, (vec2){ 60, 10 });
	ui_window_set_position(extra_window, UI_ORIGIN_BOTTOM_LEFT, (vec2){ 60, - 30 - 10 });
	{
		UIObject layout = ui_layout_new();
		{
			UIObject context_btn = ui_button_new();
			{
				UIObject context_lbl = ui_label_new();
				ui_label_set_text(context_lbl, "ctx");
				ui_label_set_color(context_lbl, (vec4){ 1.0, 1.0, 0.0, 1.0 });
				ui_label_set_alignment(context_lbl, UI_LABEL_ALIGN_CENTER);
				ui_button_set_label(context_btn, context_lbl);
			}
			ui_button_set_callback(context_btn, NULL, context_btn_cbk);
			ui_layout_append(layout, context_btn);

			UIObject general_btn = ui_button_new();
			{
				UIObject context_lbl = ui_label_new();
				ui_label_set_text(context_lbl, "gen");
				ui_label_set_color(context_lbl, (vec4){ 1.0, 1.0, 0.0, 1.0 });
				ui_label_set_alignment(context_lbl, UI_LABEL_ALIGN_CENTER);
				ui_button_set_label(general_btn, context_lbl);
			}
			ui_button_set_callback(general_btn, NULL, general_btn_cbk);
			ui_layout_append(layout, general_btn);
		}
		ui_window_append_child(extra_window, layout);
	}

	editor.controls_ui = ui_window_new();
	ui_window_set_size(editor.controls_ui, (vec2){ 90, 15 });
	ui_window_set_position(editor.controls_ui, UI_ORIGIN_BOTTOM_LEFT, (vec2){ 0 + 90, - 15 });
	ui_window_set_decorated(editor.controls_ui, false);
	
	editor.context_window = ui_window_new();
	ui_window_set_size(editor.context_window, (vec2){ 90, 60 });
	ui_window_set_position(editor.context_window, UI_ORIGIN_BOTTOM_LEFT, (vec2){ 90, - 30 - 60 });
	ui_window_set_decorated(editor.context_window, false);

	editor.general_window = ui_window_new();
	ui_window_set_size(editor.general_window, (vec2){ 90, 60 });
	ui_window_set_position(editor.general_window, UI_ORIGIN_BOTTOM_LEFT, (vec2){ 90, - 30 - 60 });
	ui_window_set_decorated(editor.general_window, false);


	editor.map_atlas = SPRITE_TERRAIN;
	if(editor.map == NULL)
		editor.map = map_alloc(16, 16);

	for(int i = 0; i < LAST_EDITOR_STATE; i++) {
		if(state_vtable[i].init)
			state_vtable[i].init();
	}

	if(state_vtable[editor.editor_state].enter) {
		state_vtable[editor.editor_state].enter();
	}

	ui_child_append(ui_root(), editor.controls_ui);
	ui_child_append(ui_root(), extra_window);
	//ui_child_append(ui_root(), editor.context_window);
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
	vec2 v;

	if(ui_is_active())
		return;

	state_vtable[editor.editor_state].mouse_button(event, v);
	update_cursor_pos(v);
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
	vec2 v;

	if(ui_is_active())
		return;
	state_vtable[editor.editor_state].mouse_motion(event, v);
	update_cursor_pos(v);
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
	arrbuf_free(&cursor_pos_str);
}

int
export_map(const char *map_file) 
{
	size_t map_data_size;
	char *map_data = map_export(editor.map, &map_data_size);
	FILE *fp = fopen(map_file, "w");
	if(!fp)
		goto error_open;
	
	int f = fwrite(map_data, 1, map_data_size, fp);
	if(f < (int)(map_data_size)) {
		printf("Error saving file: %s\n", map_file);
		goto error_save;
	} else
		printf("File saved at %s\n", map_file);
	
	fclose(fp);
	
	return 1;
error_save:
	fclose(fp);
error_open:
	free(map_data);

	return 0;
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

void
open_cbk(UIObject btn, void *userptr)
{
	(void)btn;
	(void)userptr;

	ui_child_prepend(ui_root(), *(UIObject*)userptr);
}

void
cancel_cbk(UIObject obj, void *userptr)
{
	(void)obj;
	(void)userptr;
	ui_deparent(*(UIObject*)userptr);
}

void
newbtn_new_cbk(UIObject obj, void *userptr)
{
	(void)obj;
	(void)userptr;

	int width, height;

	StrView width_text = ui_text_input_get_str(new_width);
	StrView height_text = ui_text_input_get_str(new_height);

	if(!strview_int(width_text, &width))
		return;

	if(!strview_int(height_text, &height))
		return;
	
	map_free(editor.map);
	editor.map = map_alloc(width, height);
	ui_deparent(new_window);
	ui_text_input_clear(new_width);
	ui_text_input_clear(new_height);
}

void
loadbtn_load_cbk(UIObject obj, void *userptr)
{
	(void)obj;
	(void)userptr;
	StrView path = ui_text_input_get_str(load_path);

	char *fixed_path = strview_str(path);
	
	Map *n_map = map_load(fixed_path);

	if(!n_map) {
		free(fixed_path);
		return;
	}

	map_free(editor.map);
	editor.map = n_map;

	free(fixed_path);
	ui_deparent(load_window);
	ui_text_input_clear(load_path);
}

void
save_btn_cbk(UIObject obj, void *userptr)
{
	(void)obj;
	(void)userptr;
	StrView path = ui_text_input_get_str(save_path);

	char *fixed_path = strview_str(path);

	if(export_map(fixed_path)) {
		ui_deparent(save_window);
		ui_text_input_clear(save_path);
	}
	free(fixed_path);

}

void
context_btn_cbk(UIObject obj, void *ptr)
{
	(void)obj;
	(void)ptr;
	if(menu_shown == CONTEXT_MENU) {
		menu_shown = 0;
	} else {
		menu_shown = CONTEXT_MENU;
	}
	process_open_menu();
}

void
general_btn_cbk(UIObject obj, void *ptr)
{
	(void)obj;
	(void)ptr;
	if(menu_shown == GENERAL_MENU) {
		menu_shown = 0;
	} else {
		menu_shown = GENERAL_MENU;
	}
	process_open_menu();
}

void
process_open_menu(void)
{
	if(menu_shown) {
		ui_window_set_position(extra_window, UI_ORIGIN_BOTTOM_LEFT, (vec2){ 60, - 30 - 120 - 10 });
	} else {
		ui_window_set_position(extra_window, UI_ORIGIN_BOTTOM_LEFT, (vec2){ 60, - 30 - 10 });
	}

	ui_deparent(editor.context_window);
	ui_deparent(editor.general_window);
	
	switch(menu_shown) {
	case CONTEXT_MENU:
		ui_child_append(ui_root(), editor.context_window);
		break;
	case GENERAL_MENU:
		ui_child_append(ui_root(), editor.general_window);
		break;
	default:
		break;
	}
}

void
update_cursor_pos(vec2 v)
{
	arrbuf_clear(&cursor_pos_str);
	arrbuf_printf(&cursor_pos_str, "%0.2f,%0.2f\n", v[0], v[1]);

	ui_label_set_text(cursor_position, cursor_pos_str.data);
}
