#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <glad/gles2.h>

#include <SDL.h>
#include <wchar.h>

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
	},
	[EDITOR_EDIT_THINGS] = {
		.render = thing_render,
		.wheel = thing_wheel,
		.keyboard = thing_keyboard,
		.mouse_button = thing_mouse_button,
		.mouse_motion = thing_mouse_motion,
		.enter = thing_enter,
		.exit = thing_exit,
		.init = thing_init,
		.terminate = thing_terminate,
	}
};

enum {
	CONTEXT_MENU = 0x01,
	GENERAL_MENU = 0x02
};

static int menu_shown;

static UIObject *new_window, *new_width, *new_height;
static UIObject *load_window, *load_path;
static UIObject *save_window, *save_path;
static UIObject *extra_window;
static UIObject *cursor_position;

static UIObject *tileset_window;

static ArrayBuffer cursor_pos_str;

static vec2 camera_offset;
static float camera_zoom = 16.0;

static void open_cbk(UIObject *obj, void (*userptr));
static void play_cbk(UIObject *obj, void (*userptr));
static void cancel_cbk(UIObject *btn, void *userptr);
static void newbtn_new_cbk(UIObject *btn, void *userptr);
static void loadbtn_load_cbk(UIObject *btn, void *userptr);
static void save_btn_cbk(UIObject *btn, void *userptr);

static void tileselect_cbk(UIObject *obj, void *userptr);
static void tileset_btn_cbk(UIObject *obj, void *ptr);

static void context_btn_cbk(UIObject *btn, void *userptr);
static void general_btn_cbk(UIObject *btn, void *userptr);

static void process_open_menu(void);
static void update_cursor_pos(vec2 v);

static void select_mode_cbk(UIObject *obj, void *ptr)
{
	(void)obj;

	EditorState mode = (State*)ptr - state_vtable;
	editor_change_state(mode);
}

void
GAME_STATE_LEVEL_EDIT_init(void)
{
	arrbuf_init(&cursor_pos_str);

	UIObject *layout = ui_layout_new();
	ui_layout_set_order(layout, UI_LAYOUT_HORIZONTAL);

	#define BUTTON_MODE(MODE, ICON_X, ICON_Y) \
	{ \
		UIObject *btn = ui_button_new(); \
		UIObject *img = ui_image_new();  \
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

	BUTTON_MODE(EDITOR_EDIT_COLLISION, 3 * 8, 0 * 8);
	BUTTON_MODE(EDITOR_EDIT_THINGS, 5 * 8, 0 * 8);

	#undef BUTTON_MODE

	tileset_window = ui_window_new();
	ui_window_set_decorated(tileset_window, false);
	ui_window_set_size(tileset_window, (vec2){ 150, 150 });
	ui_window_set_position(tileset_window, UI_ORIGIN_BOTTOM_RIGHT, (vec2){ - 150, - 150 });
	{
		UIObject *tileset = ui_tileset_sel_new();
		ui_tileset_sel_set_cbk(tileset, NULL, tileselect_cbk);

		ui_window_append_child(tileset_window, tileset);
	}

	UIObject *tileset_btn_window = ui_window_new();
	ui_window_set_decorated(tileset_btn_window, false);
	ui_window_set_size(tileset_btn_window, (vec2){ 30, 10 });
	ui_window_set_position(tileset_btn_window, UI_ORIGIN_BOTTOM_RIGHT, (vec2){ -30, - 10 });
	{
		UIObject *tileset_btn = ui_button_new();
		ui_button_set_callback(tileset_btn, NULL, tileset_btn_cbk);
		{
			UIObject *tileset_lbl = ui_label_new();

			ui_label_set_color(tileset_lbl, (vec4){ 1.0, 1.0, 0.0, 1.0 });
			ui_label_set_text(tileset_lbl, "Tiles");
			ui_label_set_alignment(tileset_lbl, UI_LABEL_ALIGN_CENTER);
			ui_button_set_label(tileset_btn, tileset_lbl);
		}
		ui_window_append_child(tileset_btn_window, tileset_btn);
	}
	ui_child_append(ui_root(), tileset_btn_window);

	UIObject *window = ui_window_new();
	ui_window_set_size(window, (vec2){ 80, 30 });
	ui_window_set_position(window, UI_ORIGIN_TOP_LEFT, (vec2){ 80, 30 });
	ui_window_set_border(window, (vec2){ 2, 2 });
	ui_window_set_decorated(window, false);
	ui_window_append_child(window, layout);
	ui_child_append(ui_root(), window);

	UIObject *cursor_position_window = ui_window_new();
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

	UIObject *file_buttons_window = ui_window_new();
	ui_window_set_decorated(file_buttons_window, false);
	ui_window_set_size(file_buttons_window, (vec2){ 40, 40 });
	ui_window_set_position(file_buttons_window, UI_ORIGIN_TOP_RIGHT, (vec2){ -40, 40 });
	{
		UIObject *layout = ui_layout_new();
		ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
		{
			UIObject *btn;

			#define CREATE_BUTTON(name, userptr, cbk) \
			btn = ui_button_new(); \
			ui_button_set_callback(btn, userptr, cbk); \
			{\
				UIObject *lbl = ui_label_new(); \
				ui_label_set_text(lbl, name); \
				ui_label_set_color(lbl, (vec4){ 1.0, 1.0, 0.0, 1.0 }); \
				ui_button_set_label(btn, lbl); \
			} \
			ui_layout_append(layout, btn)
			
			CREATE_BUTTON("Save", &save_window, open_cbk);
			CREATE_BUTTON("Load", &load_window, open_cbk);
			CREATE_BUTTON("New", &new_window, open_cbk);
			CREATE_BUTTON("Play", NULL, play_cbk);
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
		UIObject *layout = ui_layout_new();
		ui_layout_set_border(layout, 30, 30, 20, 20);
		ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
		ui_layout_set_fixed_size(layout, 20.0);
		{
			UIObject *subv, *lbl;
			
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
				UIObject *new = ui_button_new();
				ui_button_set_callback(new, NULL, newbtn_new_cbk);
				{
					UIObject *label = ui_label_new();
					ui_label_set_text(label, "New");
					ui_label_set_alignment(label, UI_LABEL_ALIGN_CENTER);
					ui_button_set_label(new, label);
				}
				ui_child_append(subv, new);

				UIObject *cancel = ui_button_new();
				ui_button_set_callback(cancel, &new_window, cancel_cbk);
				
				{
					UIObject *label = ui_label_new();
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
		UIObject *layout = ui_layout_new();
		ui_layout_set_border(layout, 30, 30, 20, 20);
		ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
		ui_layout_set_fixed_size(layout, 20.0);
		{
			UIObject *subv, *lbl;
			
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
				UIObject *load = ui_button_new();
				ui_button_set_callback(load, NULL, loadbtn_load_cbk);
				{
					UIObject *label = ui_label_new();
					ui_label_set_text(label, "Load");
					ui_label_set_alignment(label, UI_LABEL_ALIGN_CENTER);
					ui_button_set_label(load, label);
				}
				ui_child_append(subv, load);

				UIObject *cancel = ui_button_new();
				ui_button_set_callback(cancel, &load_window, cancel_cbk);
				
				{
					UIObject *label = ui_label_new();
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
		UIObject *layout = ui_layout_new();
		ui_layout_set_border(layout, 30, 30, 20, 20);
		ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
		ui_layout_set_fixed_size(layout, 20.0);
		{
			UIObject *subv, *lbl;
			
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
				UIObject *save = ui_button_new();
				ui_button_set_callback(save, NULL, save_btn_cbk);
				{
					UIObject *label = ui_label_new();
					ui_label_set_text(label, "Save");
					ui_label_set_alignment(label, UI_LABEL_ALIGN_CENTER);
					ui_button_set_label(save, label);
				}
				ui_child_append(subv, save);

				UIObject *cancel = ui_button_new();
				ui_button_set_callback(cancel, &save_window, cancel_cbk);
				{
					UIObject *label = ui_label_new();
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
		UIObject *layout = ui_layout_new();
		{
			UIObject *context_btn = ui_button_new();
			{
				UIObject *context_lbl = ui_label_new();
				ui_label_set_text(context_lbl, "ctx");
				ui_label_set_color(context_lbl, (vec4){ 1.0, 1.0, 0.0, 1.0 });
				ui_label_set_alignment(context_lbl, UI_LABEL_ALIGN_CENTER);
				ui_button_set_label(context_btn, context_lbl);
			}
			ui_button_set_callback(context_btn, NULL, context_btn_cbk);
			ui_layout_append(layout, context_btn);

			UIObject *general_btn = ui_button_new();
			{
				UIObject *context_lbl = ui_label_new();
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
	if(editor.map == NULL) {
		editor.map = map_alloc(16, 16);
		for(int i = 0; i < 64; i++) {
			Thing *thing = calloc(1, sizeof(*thing));
			thing->type = THING_WORLD_MAP;
			thing->layer = i;
			map_insert_thing(editor.map, thing);
			editor.layers[i] = thing;
		}
	}

	for(int i = 0; i < LAST_EDITOR_STATE; i++) {
		if(state_vtable[i].init)
			state_vtable[i].init();
	}

	if(state_vtable[editor.editor_state].enter) {
		state_vtable[editor.editor_state].enter();
	}

	ui_child_append(ui_root(), editor.controls_ui);
	ui_child_append(ui_root(), extra_window);
	
	gfx_set_camera(camera_offset, (vec2){ camera_zoom, camera_zoom });
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
	vec2 v;
	gfx_pixel_to_world((vec2){ event->button.x, event->button.y }, v);

	state_vtable[editor.editor_state].mouse_button(event);
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
	if(ui_is_active())
		return;
	vec2 v;
	gfx_pixel_to_world((vec2){ event->motion.x, event->motion.y }, v);

	state_vtable[editor.editor_state].mouse_motion(event);
	update_cursor_pos(v);
}

void
GAME_STATE_LEVEL_EDIT_keyboard(SDL_Event *event)
{
	if(ui_is_active())
		return;

	if(state_vtable[editor.editor_state].keyboard)
		state_vtable[editor.editor_state].keyboard(event);
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
	ui_reset();
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
open_cbk(UIObject *btn, void *userptr)
{
	(void)btn;
	(void)userptr;

	ui_child_prepend(ui_root(), *(UIObject**)userptr);
}

void
cancel_cbk(UIObject *obj, void *userptr)
{
	(void)obj;
	(void)userptr;
	ui_deparent(*(UIObject**)userptr);
}

void
newbtn_new_cbk(UIObject *obj, void *userptr)
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

	editor_change_state(editor.editor_state);
}

void
loadbtn_load_cbk(UIObject *obj, void *userptr)
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
	Thing *layer = editor.map->things;
	for(int i = 0; i < 64; i++) {
		editor.layers[i] = layer;
		layer = layer->next;
	}

	free(fixed_path);
	ui_deparent(load_window);
	ui_text_input_clear(load_path);
	editor_change_state(editor.editor_state);
}

void
save_btn_cbk(UIObject *obj, void *userptr)
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
context_btn_cbk(UIObject *obj, void *ptr)
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
general_btn_cbk(UIObject *obj, void *ptr)
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

void
editor_move_camera(vec2 delta)
{
	vec2_sub(camera_offset, camera_offset, delta);
	gfx_set_camera(camera_offset, (vec2){ camera_zoom, camera_zoom });
}

void
editor_delta_zoom(float delta)
{
	camera_zoom += delta;
	if(camera_zoom < 1.0)
		camera_zoom = 1.0;
	gfx_set_camera(camera_offset, (vec2){ camera_zoom, camera_zoom });
}

float
editor_get_zoom(void)
{
	return camera_zoom;
}

void 
play_cbk(UIObject *obj, void (*userptr))
{
	(void)obj;
	(void)userptr;
	gstate_set(GAME_STATE_LEVEL);
}

void
common_draw_map(int current_layer, float after_layer_alpha)
{
	TextureStamp stamp;
	int rows, cols;

	gfx_sprite_count_rows_cols(SPRITE_TERRAIN, &rows, &cols);

	for(int k = 0; k < SCENE_LAYERS; k++)
	for(int i = 0; i < editor.map->w * editor.map->h; i++) {
		float x = (     (i % editor.map->w) + 0.5);
		float y = ((int)(i / editor.map->w) + 0.5);

		int spr = editor.map->tiles[i + k * editor.map->w * editor.map->h] - 1;
		if(spr < 0)
			continue;
		int spr_x = spr % cols;
		int spr_y = spr / cols;

		stamp = get_sprite(SPRITE_TERRAIN, spr_x, spr_y);

		gfx_push_texture_rect(
				&stamp,
				(vec2){ x, y },
				(vec2){ 0.5, 0.5 }, (vec2){ 1.0, 1.0 },
				0.0,
				(vec4){ 1.0, 1.0, 1.0, k <= current_layer ? 1.0 : after_layer_alpha }
		);
	}
}

void
tileset_btn_cbk(UIObject *obj, void *ptr)
{
	(void)ptr;
	obj = ui_get_parent(ui_get_parent(obj));

	if(ui_get_parent(tileset_window) == 0) {
		ui_child_append(ui_root(), tileset_window);
		ui_window_set_position(obj, UI_ORIGIN_BOTTOM_RIGHT, (vec2){ -30, -10 - 300 });
	} else {
		ui_deparent(tileset_window);
		ui_window_set_position(obj,UI_ORIGIN_BOTTOM_RIGHT, (vec2){ -30, -10 });
	}
}

void 
tileselect_cbk(UIObject *obj, void *userptr)
{
	(void)userptr;

	editor.current_tile = ui_tileset_sel_get_selected(obj);
}
