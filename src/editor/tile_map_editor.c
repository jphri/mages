#include <stdio.h>
#include <stdbool.h>
#include <glad/gles2.h>

#include <SDL.h>
#include <wchar.h>

#include "util.h"

#include "../game_state.h"
#include "vecmath.h"
#include "graphics.h"
#include "map.h"
#include "../ui.h"
#include "editor.h"

typedef void (*ThingRender)(Thing *thing);

EditorGlobal editor;

enum {
	THING_MENU = 0x01,
	BRUSH_MENU = 0x02,
	GENERAL_MENU = 0x04,
};

static void layer_slider_cbk(UIObject *obj, void *userptr);

static void select_thing(Thing *thing);
static void select_brush(MapBrush *brush);

static void rect_begin(int x, int y);
static void rect_drag(int x, int y);
static void rect_end(int x, int y);
static void rect_draw(void);

static void proc_select_begin(int x, int y);
static void proc_select_end(int x, int y);
static void proc_select_drag(int x, int y);

static void flip_cbk(UIObject *obj, void *userptr);
static void thing_type_name_cbk(UIObject *obj, void *userptr);
static void thing_float(UIObject *obj, void *userptr);
static void thing_int(UIObject *obj, void *userptr);
static void thing_direction(UIObject *obj, void *userptr);
static void update_inputs_things(void);
static void update_inputs_brush(void);

static void brush_float_cbk(UIObject *object, void *userptr);
static void brush_check_cbk(UIObject *object, void *userptr);
static void brush_int_cbk(UIObject *object, void *userptr);
static void brush_btn_cbk(UIObject *object, void *userptr);

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

static void thing_nothing_render(Thing *thing);
static void thing_null_render(Thing *thing);
static void thing_player_render(Thing *thing);
static void thing_dummy_render(Thing *thing);
static void render_thing(Thing *thing);

static void init(void);
static void end(void);
static void mouse_button(SDL_Event *event);
static void mouse_wheel(SDL_Event *event);
static void mouse_move(SDL_Event *event);
static void keyboard(SDL_Event *event);
static void render(int w, int h);

static GameStateVTable state_vtable = {
	.init = init,
	.end = end,
	.render = render,
	.mouse_button = mouse_button,
	.mouse_wheel = mouse_wheel,
	.mouse_move = mouse_move,
	.keyboard = keyboard,
};

static ThingRender renders[LAST_THING] = {
	[THING_NULL] = thing_null_render,
	[THING_WORLD_MAP] = thing_nothing_render,
	[THING_PLAYER] = thing_player_render,
	[THING_DUMMY] = thing_dummy_render
};

static int menu_shown;

static UIObject *new_window;
static UIObject *load_window, *load_path;
static UIObject *save_window, *save_path;
static UIObject *extra_window;
static UIObject *cursor_position;

static UIObject *tileset_window;

static ArrayBuffer cursor_pos_str;

static vec2 camera_offset;
static float camera_zoom = 16.0;

static vec2  begin_offset, move_offset, offset;
static int current_layer = 0;
static bool  ctrl_pressed = false, shift_pressed = false;

static MouseState mouse_state;

static UIObject *general_root;
static UIObject *after_layer_alpha_slider;
static UIObject *integer_round, *collidable;

static MapBrush *selected_brush;
static Thing *selected_thing;

static StrView type_string[LAST_THING];
static UIObject *thing_context, *thing_type_name;
static UIObject *uiposition_x, *uiposition_y;
static UIObject *uihealth, *uihealth_max;
static UIObject *uidirection;
static UIObject *uilayer;

static UIObject *ui_brush_root;
static UIObject *ui_brush_tile;
static UIObject *ui_brush_collidable;
static UIObject *ui_brush_pos_x, *ui_brush_pos_y;
static UIObject *ui_brush_size_x, *ui_brush_size_y;

static MapBrush copied_brush;
static Thing copied_thing;
static enum {
	CLIPBOARD_NONE, 
	CLIPBOARD_BRUSH,
	CLIPBOARD_THING
} clipboard;

static ArrayBuffer helper_print;

static const char *direction_str[] = {
	[DIR_UP] = "up",
	[DIR_LEFT] = "left",
	[DIR_DOWN] = "down",
	[DIR_RIGHT] = "right"
};

void 
start_game_level_edit(void)
{
	game_change_state_vtable(&state_vtable);
}

void
init(void)
{
	arrbuf_init(&cursor_pos_str);
	clipboard = CLIPBOARD_NONE;

	arrbuf_init(&helper_print);
	type_string[THING_NULL]   = to_strview("");
	type_string[THING_PLAYER] = to_strview("THING_PLAYER");
	type_string[THING_DUMMY]  = to_strview("THING_DUMMY");
	type_string[THING_DOOR]   = to_strview("THING_DOOR");
	type_string[THING_WORLD_MAP] = to_strview("THING_WORLD_MAP");

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

	UIObject *cursor_position_window = ui_window_new();
	ui_window_set_size(cursor_position_window, (vec2){ 50, 15 });
	ui_window_set_position(cursor_position_window, UI_ORIGIN_TOP_LEFT, (vec2){ 50, 15 });
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
			UIObject *subv;
			
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
			
			UIObject *lbl_sure = ui_label_new();
			ui_label_set_text(lbl_sure, "Are you sure?");
			ui_label_set_alignment(lbl_sure, UI_LABEL_ALIGN_CENTER);
			ui_layout_append(layout, lbl_sure);

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
	ui_window_set_size(extra_window, (vec2){ 90, 10 });
	ui_window_set_position(extra_window, UI_ORIGIN_BOTTOM_LEFT, (vec2){ 90, - 10 });
	{
		UIObject *layout = ui_layout_new();
		{
			UIObject *context_btn = ui_button_new();
			{
				UIObject *context_lbl = ui_label_new();
				ui_label_set_text(context_lbl, "thing");
				ui_label_set_color(context_lbl, (vec4){ 1.0, 1.0, 0.0, 1.0 });
				ui_label_set_alignment(context_lbl, UI_LABEL_ALIGN_CENTER);
				ui_button_set_label(context_btn, context_lbl);
			}
			ui_button_set_callback(context_btn, NULL, context_btn_cbk);
			ui_layout_append(layout, context_btn);

			UIObject *brush_btn = ui_button_new();
			{
				UIObject *brush_lbl = ui_label_new();
				ui_label_set_text(brush_lbl, "brush");
				ui_label_set_color(brush_lbl, (vec4){ 1.0, 1.0, 0.0, 1.0 });
				ui_label_set_alignment(brush_lbl, UI_LABEL_ALIGN_CENTER);
				ui_button_set_label(brush_btn, brush_lbl);
			}
			ui_button_set_callback(brush_btn, NULL, brush_btn_cbk);
			ui_layout_append(layout, brush_btn);

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

	general_root = ui_new_object(0, UI_ROOT);
	{
		UIObject *general_layout = ui_layout_new();
		ui_layout_set_order(general_layout, UI_LAYOUT_VERTICAL);
		{
			UIObject *sublayout, *label;

			#define BEGIN_SUB(LABEL_STRING) \
			sublayout = ui_layout_new(); \
			ui_layout_set_order(sublayout, UI_LAYOUT_HORIZONTAL); \
			ui_layout_set_border(sublayout, 2.5, 2.5, 2.5, 2.5); \
			label = ui_label_new(); \
			ui_label_set_text(label, LABEL_STRING); \
			ui_label_set_alignment(label, UI_LABEL_ALIGN_RIGHT); \
			ui_label_set_color(label, (vec4){ 1.0, 1.0, 1.0, 1.0 }); \
			ui_layout_append(sublayout, label); \

			#define END_SUB ui_layout_append(general_layout, sublayout)

			BEGIN_SUB("Layer: ") {
				UIObject *slider_size = ui_slider_new();
				ui_slider_set_min_value(slider_size, 0.0);
				ui_slider_set_max_value(slider_size, 63);
				ui_slider_set_precision(slider_size, 63);
				ui_slider_set_value(slider_size, current_layer);
				ui_slider_set_callback(slider_size, NULL, layer_slider_cbk);

				ui_layout_append(sublayout, slider_size);
			} END_SUB;

			BEGIN_SUB("Alpha After: ") {
				after_layer_alpha_slider = ui_slider_new();
				ui_slider_set_min_value(after_layer_alpha_slider, 0.0);
				ui_slider_set_max_value(after_layer_alpha_slider, 1.0);
				ui_slider_set_precision(after_layer_alpha_slider, 1024);

				ui_layout_append(sublayout, after_layer_alpha_slider);
			} END_SUB;

			BEGIN_SUB("Integer round:") {
				integer_round = ui_checkbox_new();
				ui_checkbox_set_toggled(integer_round, false);
				ui_checkbox_set_callback(integer_round, NULL, flip_cbk);

				ui_layout_append(sublayout, integer_round);
			} END_SUB;

			BEGIN_SUB("Collidable") {
				collidable = ui_checkbox_new();
				ui_checkbox_set_toggled(collidable, false);
				ui_checkbox_set_callback(collidable, NULL, flip_cbk);

				ui_layout_append(sublayout, collidable);
			} END_SUB;
		}
		ui_child_append(general_root, general_layout);
	}

	thing_context = ui_new_object(0, UI_ROOT);
	UIObject *layout = ui_layout_new();
	ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
	ui_layout_set_border(layout, 2.0, 2.0, 2.0, 2.0);
	ui_layout_set_fixed_size(layout, 10.0); \
	{
		UIObject *sublayout, *label;

		#define BEGIN_LAYOUT(NAME) \
		sublayout = ui_layout_new(); \
		ui_layout_set_order(sublayout, UI_LAYOUT_HORIZONTAL); \
		ui_layout_set_border(sublayout, 2.0, 2.0, 2.0, 2.0); \
		label = ui_label_new(); \
		ui_label_set_text(label, NAME); \
		ui_label_set_alignment(label, UI_LABEL_ALIGN_LEFT); \
		ui_layout_append(sublayout, label);

		#define END_LAYOUT \
		ui_layout_append(layout, sublayout);


		BEGIN_LAYOUT("type"); {
			thing_type_name = ui_text_input_new();
			ui_text_input_set_cbk(thing_type_name, NULL, thing_type_name_cbk);
			ui_layout_append(sublayout, thing_type_name);
		} END_LAYOUT;

		BEGIN_LAYOUT("position"); {
			UIObject *retarded = ui_layout_new(); 
			ui_layout_set_order(retarded, UI_LAYOUT_HORIZONTAL);
			{
				uiposition_x = ui_text_input_new();
				ui_text_input_set_cbk(uiposition_x, (void*)(offsetof(Thing, position[0])), thing_float);
				ui_layout_append(retarded, uiposition_x);

				uiposition_y = ui_text_input_new();
				ui_text_input_set_cbk(uiposition_y, (void*)(offsetof(Thing, position[1])), thing_float);
				ui_layout_append(retarded, uiposition_y);
			}
			ui_layout_append(sublayout, retarded);
		} END_LAYOUT;

		BEGIN_LAYOUT("health"); {
			UIObject *retarded = ui_layout_new(); 
			ui_layout_set_order(retarded, UI_LAYOUT_HORIZONTAL);
			{
				uihealth = ui_text_input_new();
				ui_text_input_set_cbk(uihealth, (void*)(offsetof(Thing, health)), thing_float);
				ui_layout_append(retarded, uihealth);
			}
			ui_layout_append(sublayout, retarded);
		} END_LAYOUT;

		BEGIN_LAYOUT("health_max"); {
			UIObject *retarded = ui_layout_new(); 
			ui_layout_set_order(retarded, UI_LAYOUT_HORIZONTAL);
			{
				uihealth_max = ui_text_input_new();
				ui_text_input_set_cbk(uihealth_max, (void*)(offsetof(Thing, health_max)), thing_float);
				ui_layout_append(retarded, uihealth_max);
			}
			ui_layout_append(sublayout, retarded);
		} END_LAYOUT;

		BEGIN_LAYOUT("direction"); {
			UIObject *retarded = ui_layout_new(); 
			ui_layout_set_order(retarded, UI_LAYOUT_HORIZONTAL);
			{
				uidirection = ui_text_input_new();
				ui_text_input_set_cbk(uidirection, (void*)(offsetof(Thing, direction)), thing_direction);
				ui_layout_append(retarded, uidirection);
			}
			ui_layout_append(sublayout, retarded);
		} END_LAYOUT;

		BEGIN_LAYOUT("layer") {
			UIObject *retarded = ui_layout_new(); 
			ui_layout_set_order(retarded, UI_LAYOUT_HORIZONTAL);
			{
				uilayer = ui_text_input_new();
				ui_text_input_set_cbk(uilayer, (void*)(offsetof(Thing, direction)), thing_int);
				ui_layout_append(retarded, uilayer);
			}
			ui_layout_append(sublayout, retarded);
		} END_LAYOUT;
	}

	ui_child_append(thing_context, layout);
	#undef BEGIN_LAYOUT
	#undef END_LAYOUT

	ui_brush_root = ui_new_object(0, UI_ROOT);
	layout = ui_layout_new();
	ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
	ui_layout_set_border(layout, 2.0, 2.0, 2.0, 2.0);
	ui_layout_set_fixed_size(layout, 10.0); \
	{
		UIObject *sublayout, *label, *sub;

		#define BEGIN_LAYOUT(NAME) \
		sublayout = ui_layout_new(); \
		ui_layout_set_order(sublayout, UI_LAYOUT_HORIZONTAL); \
		ui_layout_set_border(sublayout, 2.0, 2.0, 2.0, 2.0); \
		label = ui_label_new(); \
		ui_label_set_text(label, NAME); \
		ui_label_set_alignment(label, UI_LABEL_ALIGN_LEFT); \
		ui_layout_append(sublayout, label); \
		sub = ui_layout_new();\
		ui_layout_set_order(sub, UI_LAYOUT_HORIZONTAL); \
		ui_layout_append(sublayout, sub); \
		ui_layout_append(layout, sublayout);

		BEGIN_LAYOUT("position") {
			ui_brush_pos_x = ui_text_input_new();
			ui_text_input_set_cbk(ui_brush_pos_x, (void*)(offsetof(MapBrush, position[0])), brush_float_cbk);
			ui_layout_append(sub, ui_brush_pos_x);

			ui_brush_pos_y = ui_text_input_new();
			ui_text_input_set_cbk(ui_brush_pos_y, (void*)(offsetof(MapBrush, position[1])), brush_float_cbk);
			ui_layout_append(sub, ui_brush_pos_y);
		}

		BEGIN_LAYOUT("half_size") {
			ui_brush_size_x = ui_text_input_new();
			ui_text_input_set_cbk(ui_brush_size_x, (void*)(offsetof(MapBrush, half_size[0])), brush_float_cbk);
			ui_layout_append(sub, ui_brush_size_x);

			ui_brush_size_y = ui_text_input_new();
			ui_text_input_set_cbk(ui_brush_size_y, (void*)(offsetof(MapBrush, half_size[1])), brush_float_cbk);
			ui_layout_append(sub, ui_brush_size_y);
		}

		BEGIN_LAYOUT("tile") {
			ui_brush_tile = ui_text_input_new();
			ui_text_input_set_cbk(ui_brush_tile, (void*)(offsetof(MapBrush, tile)), brush_int_cbk);
			ui_layout_append(sub, ui_brush_tile);
		}
		
		BEGIN_LAYOUT("collidable") {
			ui_brush_collidable = ui_checkbox_new();
			ui_checkbox_set_callback(ui_brush_collidable, (void*)(offsetof(MapBrush, collidable)), brush_check_cbk);
			ui_layout_append(sub, ui_brush_collidable);
		}

	}
	ui_child_append(ui_brush_root, layout);
	
	#undef BEGIN_SUB
	#undef END_SUB
	selected_brush = NULL;

	editor.thing_window = ui_window_new();
	ui_window_set_size(editor.thing_window, (vec2){ 90, 60 });
	ui_window_set_position(editor.thing_window, UI_ORIGIN_BOTTOM_LEFT, (vec2){ 90, - 60 });
	ui_window_set_decorated(editor.thing_window, false);

	editor.brush_window = ui_window_new();
	ui_window_set_size(editor.brush_window, (vec2){ 90, 60 });
	ui_window_set_position(editor.brush_window, UI_ORIGIN_BOTTOM_LEFT, (vec2){ 90, - 60 });
	ui_window_set_decorated(editor.brush_window, false);

	editor.general_window = ui_window_new();
	ui_window_set_size(editor.general_window, (vec2){ 90, 60 });
	ui_window_set_position(editor.general_window, UI_ORIGIN_BOTTOM_LEFT, (vec2){ 90, - 60 });
	ui_window_set_decorated(editor.general_window, false);

	editor.map_atlas = SPRITE_TERRAIN;
	if(editor.map == NULL) {
		editor.map = map_alloc();
	}
	ui_window_append_child(editor.general_window, general_root);

	ui_child_append(ui_root(), extra_window);
	gfx_set_camera(camera_offset, (vec2){ camera_zoom, camera_zoom });
	ui_checkbox_set_toggled(integer_round, true);
}

void
render(int w, int h)
{
	(void)w;
	(void)h;

	gfx_clear();
	gfx_camera_set_enabled(true);
	gfx_set_camera(camera_offset, (vec2){ camera_zoom, camera_zoom });
	gfx_begin();
	for(Thing *c = editor.map->things; c; c = c->next) {
		render_thing(c);
	}
	if(mouse_state == MOUSE_DRAWING) {
		rect_draw();
	}
	gfx_flush();
	gfx_end();

	gfx_camera_set_enabled(false);
	ui_draw();
}

void
mouse_button(SDL_Event *event)
{
	Thing *thing;

	if(ui_is_active())
		return;
	vec2 v;

	gfx_pixel_to_world((vec2){ event->button.x, event->button.y }, v);
	if(ui_checkbox_get_toggled(integer_round))
		vec2_round(v, v);

	vec2 pos;
	if(event->type == SDL_MOUSEBUTTONUP) {
		switch(mouse_state) {
		case MOUSE_DRAWING: rect_end(event->button.x, event->button.y); break;
		case MOUSE_MOVING_BRUSH: proc_select_end(event->button.x, event->button.y); break;
		default:
			break;
		}
		mouse_state = MOUSE_NOTHING;
	}
	
	if(event->type == SDL_MOUSEBUTTONDOWN && mouse_state == MOUSE_NOTHING) {
		switch(event->button.button) {
		case SDL_BUTTON_RIGHT: 
			if(!ctrl_pressed) {
				rect_begin(event->button.x, event->button.y);
				mouse_state = MOUSE_DRAWING; 
			} else {
				thing = calloc(1, sizeof(*thing));
				thing->type = THING_NULL;
				vec2_dup(thing->position, v);
				map_insert_thing(editor.map, thing);
			}
	 		break;
		case SDL_BUTTON_MIDDLE: 
			move_offset[0] = event->button.x; 
			move_offset[1] = event->button.y; 
			vec2_dup(begin_offset, offset);
			mouse_state = MOUSE_MOVING; 
			break;
		case SDL_BUTTON_LEFT:
			gfx_pixel_to_world((vec2){ event->button.x, event->button.y }, pos);
			proc_select_begin(event->motion.x, event->motion.y);
			mouse_state = MOUSE_MOVING_BRUSH;
			break;
		}
	}
	update_cursor_pos(v);
}

void
mouse_wheel(SDL_Event *event)
{
	if(ui_is_active())
		return;

	if(ctrl_pressed) {
		editor_delta_zoom(event->wheel.y);
	}
}

void
mouse_move(SDL_Event *event) 
{
	if(ui_is_active())
		return;
	vec2 v;
	gfx_pixel_to_world((vec2){ event->motion.x, event->motion.y }, v);

	switch(mouse_state) {
	case MOUSE_MOVING:
		vec2_sub(v, move_offset, (vec2){ event->motion.x, event->motion.y });
		editor_move_camera(v);
		move_offset[0] = event->button.x; 
		move_offset[1] = event->button.y; 
		break;
	case MOUSE_DRAWING:
		rect_drag(event->motion.x, event->motion.y);
		break;
	case MOUSE_MOVING_BRUSH:
		proc_select_drag(event->motion.x, event->motion.y);
		break;
	default:
		do {} while(0);
	}
	update_cursor_pos(v);
}

void
keyboard(SDL_Event *event)
{
	if(ui_is_active())
		return;

	MapBrush *current_next, *current_prev;
	Thing *current_next_thing, *current_prev_thing;

	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_LCTRL: ctrl_pressed = true; break;
		case SDLK_LSHIFT: shift_pressed = true; break;
		case SDLK_c:
			if(ctrl_pressed) {
				if(selected_brush) {
					copied_brush = *selected_brush;
					clipboard = CLIPBOARD_BRUSH;
				} else if(selected_thing) {
					copied_thing = *selected_thing;
					clipboard = CLIPBOARD_THING;
				}
			}
			break;
		case SDLK_v:
			if(ctrl_pressed) {
				Thing *new_thing;
				MapBrush *new_brush;

				switch(clipboard) {
				case CLIPBOARD_THING:
					new_thing = malloc(sizeof(*new_thing));
					*new_thing = copied_thing;
					map_insert_thing(editor.map, new_thing);
					select_thing(new_thing);
					
					break;
				case CLIPBOARD_BRUSH:
					if(!selected_thing)
						return;
					new_brush = malloc(sizeof(*new_brush));
					*new_brush = copied_brush;
					
					map_thing_insert_brush(selected_thing, new_brush);
					select_brush(new_brush);
					break;
				case CLIPBOARD_NONE:
					break;
				}
			}
			break;
		case SDLK_UP:
			if(shift_pressed) {
				if(!selected_brush)
					return;
				current_next = selected_brush->next;
				if(!current_next)
					return;

				map_thing_remove_brush(selected_thing, selected_brush);
				map_thing_insert_brush_after(selected_thing, selected_brush, current_next);
				break;
			} else if(ctrl_pressed) {
				if(!selected_thing)
					return;

				current_next_thing = selected_thing->next;
				if(!current_next_thing)
					return;

				map_remove_thing(editor.map, selected_thing);
				map_insert_thing_after(editor.map, selected_thing, current_next_thing);
				break;
			} else {
				if(!selected_thing)
					return;

				if(selected_brush && selected_brush->next) {
					select_brush(selected_brush->next);
				} else {
					if(selected_thing->next) {
						select_thing(selected_thing->next);
						select_brush(selected_thing->brush_list);
					}
				}
			}

			break;
		case SDLK_DELETE:
			if(!ctrl_pressed) {
				if(!selected_brush)
					break;
				MapBrush *brush = selected_brush;
				Thing *thing = selected_thing;
				map_thing_remove_brush(thing, brush);
				select_brush(NULL);
				free(brush);

				switch(selected_thing->type) {
				case THING_WORLD_MAP:
					if(!selected_thing->brush_list) {
						map_remove_thing(editor.map, thing);
						select_thing(NULL);
						free(thing);
					}
				default:
					break;
				}
			} else {
				if(!selected_thing)
					break;

				map_remove_thing(editor.map, selected_thing);
				for(MapBrush *brush = selected_thing->brush_list, *next;
					brush;
					brush = next)
				{
					next = brush->next;
					free(brush);
				}
				free(selected_thing);
				select_thing(NULL);
			}
			break;
		case SDLK_DOWN:
			if(shift_pressed) {
				if(!selected_brush)
					return;
				current_prev = selected_brush->prev;
				if(!current_prev)
					return;

				map_thing_remove_brush(selected_thing, selected_brush);
				map_thing_insert_brush_before(selected_thing, selected_brush, current_prev);
				break;
			} else if(ctrl_pressed) {
				if(!selected_thing)
					return;

				current_prev_thing = selected_thing->prev;
				if(!current_prev_thing)
					return;

				map_remove_thing(editor.map, selected_thing);
				map_insert_thing_before(editor.map, selected_thing, current_prev_thing);
				break;
			} else {
				if(!selected_thing)
					return;

				if(selected_brush && selected_brush->prev) {
					select_brush(selected_brush->prev);
				} else {
					if(selected_thing->prev) {
						select_thing(selected_thing->prev);
						select_brush(selected_thing->brush_list_end);
					}
				}
			}
			break;
		}
		
	} else {
		switch(event->key.keysym.sym) {
		case SDLK_LCTRL: ctrl_pressed = false; break;
		case SDLK_LSHIFT: shift_pressed = false; break;
		}
	}
}

void 
end(void) 
{
	ui_del_object(general_root);
	ui_del_object(thing_context);
	arrbuf_free(&cursor_pos_str);
	arrbuf_free(&helper_print);
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

	map_free(editor.map);
	editor.map = map_alloc();
	select_brush(NULL);
	select_thing(NULL);
	ui_deparent(new_window);
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
	free(fixed_path);
	ui_deparent(load_window);
	ui_text_input_clear(load_path);
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
	if(menu_shown == THING_MENU) {
		menu_shown = 0;
	} else {
		menu_shown = THING_MENU;
	}
	process_open_menu();
}

void
brush_btn_cbk(UIObject *obj, void *ptr)
{
	(void)obj;
	(void)ptr;
	if(menu_shown == BRUSH_MENU) {
		menu_shown = 0;
	} else {
		menu_shown = BRUSH_MENU;
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
		ui_window_set_position(extra_window, UI_ORIGIN_BOTTOM_LEFT, (vec2){ 90, -120 - 10 });
	} else {
		ui_window_set_position(extra_window, UI_ORIGIN_BOTTOM_LEFT, (vec2){ 90, -10 });
	}

	ui_deparent(editor.thing_window);
	ui_deparent(editor.general_window);
	ui_deparent(editor.brush_window);
	
	switch(menu_shown) {
	case THING_MENU:
		ui_child_append(ui_root(), editor.thing_window);
		break;
	case GENERAL_MENU:
		ui_child_append(ui_root(), editor.general_window);
		break;
	case BRUSH_MENU:
		ui_child_append(ui_root(), editor.brush_window);
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
	start_game_level();
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

void
rect_begin(int x, int y)
{
	if(!selected_thing) {
		Thing *thing = calloc(1, sizeof(*selected_thing));
		thing->type = THING_WORLD_MAP;
		map_insert_thing(editor.map, thing);
		select_thing(thing);
	}

	gfx_pixel_to_world((vec2){ x, y }, begin_offset);
	if(ui_checkbox_get_toggled(integer_round))
		vec2_round(begin_offset, begin_offset);

	switch(selected_thing->type) {
	case THING_WORLD_MAP:
		break;
	default:
		return;
	}

	if(shift_pressed) {
		if(!selected_brush)
			return;
	} else {
		MapBrush *brush = calloc(1, sizeof(MapBrush));
		brush->half_size[0] = 0;
		brush->half_size[1] = 0;
		brush->tile = editor.current_tile;
		brush->next = NULL;
		brush->prev = NULL;
		brush->collidable = ui_checkbox_get_toggled(collidable);
		vec2_dup(brush->position, begin_offset);
		map_thing_insert_brush(selected_thing, brush);
		select_brush(brush);
	}
}

void
rect_drag(int x, int y)
{
	vec2 delta;
	vec2 v;

	if(!selected_brush)
		return;

	gfx_pixel_to_world((vec2){ x, y }, v);
	if(ui_checkbox_get_toggled(integer_round))
		vec2_round(v, v);

	vec2_sub(delta, v, begin_offset);
	vec2_add_scaled(selected_brush->position, selected_brush->position, delta, 0.5);
	vec2_add_scaled(selected_brush->half_size, selected_brush->half_size, delta, 0.5);

	vec2_dup(begin_offset, v);
	update_inputs_brush();
}

void
rect_end(int x, int y)
{
	(void)x;
	(void)y;

	if(!selected_brush)
		return;

	selected_brush->half_size[0] = fabsf(selected_brush->half_size[0]);
	selected_brush->half_size[1] = fabsf(selected_brush->half_size[1]);

	if(selected_brush->half_size[0] < 1.0 / 64.0 || selected_brush->half_size[1] < 1.0 / 64.0) {
		map_thing_remove_brush(selected_thing, selected_brush);
		free(selected_brush);
		selected_brush = NULL;
	}
}

void
rect_draw(void)
{
	int rows, cols, tile;
	if(!selected_brush)
		return;

	gfx_sprite_count_rows_cols(SPRITE_TERRAIN, &rows, &cols);

	tile = selected_brush->tile - 1;
	TextureStamp stamp = get_sprite(SPRITE_TERRAIN, tile % cols, tile / cols);
	gfx_push_texture_rect(
		&stamp, 
		selected_brush->position, 
		selected_brush->half_size, 
		(vec2){ selected_brush->half_size[0] * 2.0, selected_brush->half_size[1] * 2.0 }, 
		0.0, 
		(vec4){ 1.0, 1.0, 1.0, 1.0 });
		gfx_push_rect(selected_brush->position, selected_brush->half_size, 0.5/(editor_get_zoom()), (vec4){ 1.0, 1.0, 1.0, 1.0 });
	
}

void
flip_cbk(UIObject *obj, void *userptr)
{
	(void)userptr;
	ui_checkbox_set_toggled(obj, !ui_checkbox_get_toggled(obj));
}

void
proc_select_begin(int x, int y)
{
	vec2 p;
	Rectangle rect;
	gfx_pixel_to_world((vec2){ x, y }, p);
	
	if(selected_brush) {
		vec2_dup(rect.position, selected_brush->position);
		vec2_dup(rect.half_size, selected_brush->half_size);

		if(rect_contains_point(&rect, p)) {
			goto end_search;
		}
	}
	
	if(selected_thing) {
		vec2_dup(rect.position, selected_thing->position);
		vec2_dup(rect.half_size, (vec2){ 0.5, 0.5 });
		if(rect_contains_point(&rect, p)) {
			goto end_search;
		}
	}

	select_thing(NULL);
	select_brush(NULL);
	for(Thing *thing = editor.map->things_end; thing; thing = thing->prev) {

		vec2_dup(rect.position, thing->position);
		vec2_dup(rect.half_size, (vec2){ 0.5, 0.5 });

		if(renders[thing->type] != thing_nothing_render)
			if(rect_contains_point(&rect, p)) {
				select_thing(thing);
				goto end_search;
			}

		for(MapBrush *b = thing->brush_list_end; b; b = b->prev) {
			Rectangle rect = {
				.position = { b->position[0], b->position[1] },
				.half_size = { b->half_size[0], b->half_size[1] }
			};

			if(rect_contains_point(&rect, p)) {
				select_thing(thing);
				select_brush(b);
				goto end_search;
			}
		}
	}

end_search:
	gfx_pixel_to_world((vec2){ x, y }, begin_offset);
	if(ui_checkbox_get_toggled(integer_round))
		vec2_round(begin_offset, begin_offset);
}

void
proc_select_drag(int x, int y)
{
	if(!selected_thing)
		return;
	
	vec2 v, p;
	gfx_pixel_to_world((vec2){ x, y }, v);
	if(ui_checkbox_get_toggled(integer_round))
		vec2_round(v, v);

	vec2_sub(p, begin_offset, v);
	vec2_dup(begin_offset, v);

	if(selected_brush) {
		if(shift_pressed) {
			for(MapBrush *b = selected_thing->brush_list; b; b = b->next)
				vec2_sub(b->position, b->position, p);
		} else
			vec2_sub(selected_brush->position, selected_brush->position, p);
	} else {
		vec2_sub(selected_thing->position, selected_thing->position, p);
	}
	update_inputs_things();
	update_inputs_brush();
}

void
proc_select_end(int x, int y)
{
	(void)x;
	(void)y;
}

void
layer_slider_cbk(UIObject *obj, void *userptr)
{
	(void)userptr;
	current_layer = ui_slider_get_value(obj);
}

void 
render_thing(Thing *thing)
{
	int rows, cols;
	gfx_sprite_count_rows_cols(SPRITE_TERRAIN, &rows, &cols);

	if(renders[thing->type]) {
		renders[thing->type](thing);
	} else {
		thing_null_render(thing);
	}

	for(MapBrush *brush = thing->brush_list; brush; brush = brush->next) {
		int tile = brush->tile - 1;
		Rectangle test_rect;

		TextureStamp stamp = get_sprite(SPRITE_TERRAIN, tile % cols, tile / cols);
		vec4 color = { 1.0, 1.0, 1.0, 1.0 };

		if(selected_brush && selected_brush != brush) {
			vec2_dup(test_rect.position, brush->position);
			vec2_add(test_rect.half_size, brush->half_size, selected_brush->half_size);

			if(rect_contains_point(&test_rect, selected_brush->position)) {
				color[3] = 0.25;
			}
		} 

		if(selected_brush == brush) {
			color[1] = 0.0;
			color[2] = 0.0;
		} else if(selected_thing == thing) {
			color[2] = 0.0;
		}

		gfx_push_texture_rect(
			&stamp, 
			brush->position, 
			brush->half_size,
			(vec2){ brush->half_size[0] * 2.0, brush->half_size[1] * 2.0 }, 
			0.0, 
			(vec4){ 1.0, 1.0, 1.0, color[3] });

		gfx_push_rect(brush->position, brush->half_size, 0.5/(editor_get_zoom()), color);
	}
}

void
thing_nothing_render(Thing *c)
{
	(void)c;
}

void
thing_null_render(Thing *c)
{
	gfx_push_texture_rect(gfx_white_texture(), c->position, (vec2){ 0.5, 0.5 }, (vec2){ 1.0, 1.0 }, 0.0, (vec4){ 1.0, 0.0, 0.0, 1.0 });

	if(c == selected_thing) {
		gfx_push_texture_rect(gfx_white_texture(), c->position, (vec2){ 0.5, 0.5 }, (vec2){ 1.0, 1.0 }, 0.0, (vec4){ 0.0, 0.0, 1.0, 0.5 });
	}
}

void
thing_player_render(Thing *c)
{
	TextureStamp stamp = get_sprite(SPRITE_ENTITIES, 0, 0);
	gfx_push_texture_rect(&stamp, c->position, (vec2){ 0.5, 0.5 }, (vec2){ 1.0, 1.0 }, 0.0, (vec4){ 1.0, 1.0, 1.0, 1.0 });

	if(c == selected_thing) {
		gfx_push_texture_rect(gfx_white_texture(), c->position, (vec2){ 0.5, 0.5 }, (vec2){ 1.0, 1.0 }, 0.0, (vec4){ 0.0, 0.0, 1.0, 0.5 });
	}
}

void
thing_dummy_render(Thing *c)
{
	TextureStamp stamp = get_sprite(SPRITE_ENTITIES, 0, 2);
	gfx_push_texture_rect(&stamp, c->position, (vec2){ 0.5, 0.5 }, (vec2){ 1.0, 1.0 }, 0.0, (vec4){ 1.0, 1.0, 1.0, 1.0 });

	if(c == selected_thing) {
		gfx_push_texture_rect(gfx_white_texture(), c->position, (vec2){ 0.5, 0.5 }, (vec2){ 1.0, 1.0 }, 0.0, (vec4){ 0.0, 0.0, 1.0, 0.5 });
	}
}

void
thing_type_name_cbk(UIObject *obj, void *userptr)
{
	(void)userptr;
	selected_thing->type = THING_NULL;
	StrView v = ui_text_input_get_str(obj);
	
	for(int i = 0; i < LAST_THING; i++) {
		if(strview_cmpstr(v, type_string[i]) == 0) {
			selected_thing->type = i;
			update_inputs_things();
			return;
		}
	}
}

void
thing_float(UIObject *obj, void *userptr)
{
	float *ptr = (void*)((uintptr_t)selected_thing + (uintptr_t)userptr);
	strview_float(ui_text_input_get_str(obj), ptr);
}

void
thing_int(UIObject *obj, void *userptr)
{
	int *ptr = (void*)((uintptr_t)selected_thing + (uintptr_t)userptr);
	strview_int(ui_text_input_get_str(obj), ptr);
}

void
thing_direction(UIObject *obj, void *userptr)
{
	Direction *dir = (void*)((uintptr_t)selected_thing + (uintptr_t)userptr);
	for(size_t i = 0; i < LENGTH(direction_str); i++) {
		if(strview_cmp(ui_text_input_get_str(obj), direction_str[i]) == 0) {
			*dir = i;
			return;
		}
	}
}

void
update_inputs_things(void)
{
	#define SETINPUT(INPUT, FORMAT, COMPONENT) \
	arrbuf_clear(&helper_print); \
	arrbuf_printf(&helper_print, FORMAT, COMPONENT); \
	ui_text_input_set_text(INPUT, to_strview_buffer(helper_print.data, helper_print.size));

	if(!selected_thing)
		return;
	
	SETINPUT(uiposition_x, "%0.2f", selected_thing->position[0]);
	SETINPUT(uiposition_y, "%0.2f", selected_thing->position[1]);
	SETINPUT(uihealth, "%0.2f", selected_thing->health);
	SETINPUT(uihealth_max, "%0.2f", selected_thing->health_max);
	SETINPUT(uilayer, "%d", selected_thing->layer);

	if(selected_thing->direction >= 0 && selected_thing->direction <= 3) {
		SETINPUT(uidirection, "%s", direction_str[selected_thing->direction]);
	} else {
		SETINPUT(uidirection, "%s", "");
	}

	#undef SETINPUT
}

void
update_inputs_brush(void)
{
	#define SETINPUT(INPUT, FORMAT, COMPONENT) \
	arrbuf_clear(&helper_print); \
	arrbuf_printf(&helper_print, FORMAT, COMPONENT); \
	ui_text_input_set_text(INPUT, to_strview_buffer(helper_print.data, helper_print.size));

	if(!selected_brush)
		return;

	SETINPUT(ui_brush_pos_x, "%0.2f", selected_brush->position[0]);
	SETINPUT(ui_brush_pos_y, "%0.2f", selected_brush->position[1]);
	SETINPUT(ui_brush_size_x, "%0.2f", selected_brush->half_size[0]);
	SETINPUT(ui_brush_size_y, "%0.2f", selected_brush->half_size[1]);
	SETINPUT(ui_brush_tile, "%d", selected_brush->tile);
	ui_checkbox_set_toggled(ui_brush_collidable, selected_brush->collidable);

	#undef SETINPUT
}

void
brush_float_cbk(UIObject *obj, void *userptr)
{
	float *ptr = (void*)((uintptr_t)selected_brush + (uintptr_t)userptr);
	strview_float(ui_text_input_get_str(obj), ptr);
}

void
brush_int_cbk(UIObject *obj, void *userptr)
{
	int *ptr = (void*)((uintptr_t)selected_brush + (uintptr_t)userptr);
	strview_int(ui_text_input_get_str(obj), ptr);
}

void
brush_check_cbk(UIObject *obj, void *userptr)
{
	int *ptr = (void*)((uintptr_t)selected_brush + (uintptr_t)userptr);
	*ptr = !*ptr;

	ui_checkbox_set_toggled(obj, *ptr);
}

void
select_thing(Thing *thing)
{
	selected_thing = thing;
	update_inputs_things();

	if(selected_thing) {
		ui_text_input_set_text(thing_type_name, type_string[selected_thing->type]);
		ui_window_append_child(editor.thing_window, thing_context);
	} else {
		ui_deparent(thing_context);
	}
}

void
select_brush(MapBrush *brush)
{
	selected_brush = brush;
	update_inputs_brush();

	if(selected_brush) {
		ui_window_append_child(editor.brush_window, ui_brush_root);
	} else {
		ui_deparent(ui_brush_root);
	}
}
