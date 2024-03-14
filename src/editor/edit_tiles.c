#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>

#include "../global.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"
#include "../graphics.h"
#include "../util.h"
#include "../map.h"
#include "../ui.h"
#include "editor.h"

#define SLIDER_SIZE_PRECISION 31
#define MAX_LAYERS 64

typedef enum CursorMode {
	CURSOR_MODE_PENCIL,
	CURSOR_MODE_FILL,
	LAST_CURSOR_MODE
} CursorMode;

typedef struct {
	void (*apply)(int x, int y);
	void (*preview)(int x, int y);
} Cursor;

typedef void (*CursorApply)(int x, int y);

static void pencil_apply(int x, int y);
static void fill_apply(int x, int y);

static void pencil_preview(int x, int y);
static void fill_preview(int x, int y);

static Cursor cursors[] = {
	[CURSOR_MODE_PENCIL] = { 
		pencil_apply,
		pencil_preview,
	},
	[CURSOR_MODE_FILL] = { 
		fill_apply,
		fill_preview,
	}
};

static struct {
	TextureStamp image;
} cursor_info[] = {
	[CURSOR_MODE_PENCIL] = { .image = { .texture = TEXTURE_UI, .position = { 0.0 / 256.0, 16.0 / 256.0 }, .size = { 8.0 / 256.0, 8.0 / 256.0 } } },
	[CURSOR_MODE_FILL]   = { .image = { .texture = TEXTURE_UI, .position = { 8.0 / 256.0, 16.0 / 256.0 }, .size = { 8.0 / 256.0, 8.0 / 256.0 } } },
};

static bool  ctrl_pressed = false;
static vec2  begin_offset, move_offset, offset;
static int current_layer = 0;
static MouseState mouse_state;
static vec2 mouse_position;

static UIObject *cursor_layout;
static UIObject *context_root, *context_layout;
static UIObject *general_root;
static UIObject *tileset_window;
static UIObject *after_layer_alpha_slider;
static CursorMode cursor_mode;
static float cursor_mode_size;

static UIObject *tiles_root;

static UIObject *cursor_checkboxes[LAST_CURSOR_MODE];

static ArrayBuffer fill_preview_stack;
static ArrayBuffer fill_layer_helper;

static void layer_slider_cbk(UIObject *slider, void *userptr);
static void cursor_size_cbk(UIObject *obj, void *userptr);
static void cursorchb_cbk(UIObject *obj, void *userptr);
static void tileselect_cbk(UIObject *obj, void *userptr);

static void tileset_btn_cbk(UIObject *obj, void *ptr);

static void draw_preview(void);
static void apply_cursor(int x, int y);

void
edit_keyboard(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_LCTRL: ctrl_pressed = true; break;
		}
	} else {
		switch(event->key.keysym.sym) {
		case SDLK_LCTRL: ctrl_pressed = false; break;
		}
	}
}

void
edit_mouse_motion(SDL_Event *event)
{
	vec2 v;
	gfx_pixel_to_world((vec2){ event->button.x, event->button.y }, mouse_position);
	switch(mouse_state) {
	case MOUSE_MOVING:
		vec2_sub(v, move_offset, (vec2){ event->motion.x, event->motion.y });
		editor_move_camera(v);
		move_offset[0] = event->button.x; 
		move_offset[1] = event->button.y; 
		break;
	case MOUSE_DRAWING:
		apply_cursor(mouse_position[0], mouse_position[1]);
		break;
	default:
		do {} while(0);
	}
}

void
edit_mouse_button(SDL_Event *event)
{
	SDL_Event fake_event;
	if(event->type == SDL_MOUSEBUTTONUP) {
		mouse_state = MOUSE_NOTHING;
	}
	gfx_pixel_to_world((vec2){ event->button.x, event->button.y }, mouse_position);

	if(event->type == SDL_MOUSEBUTTONDOWN && mouse_state == MOUSE_NOTHING) {
		switch(event->button.button) {
		case SDL_BUTTON_LEFT: 
			fake_event.type = SDL_MOUSEMOTION;
			fake_event.motion.x = event->button.x;
			fake_event.motion.y = event->button.y;
			mouse_state = MOUSE_DRAWING; 
			edit_mouse_motion(&fake_event);
			break;
		case SDL_BUTTON_RIGHT: 
			move_offset[0] = event->button.x; 
			move_offset[1] = event->button.y; 
			vec2_dup(begin_offset, offset);
			mouse_state = MOUSE_MOVING; 
			break;
		}
	}
}

void
edit_wheel(SDL_Event *event)
{
	if(ctrl_pressed) {
		editor_delta_zoom(event->wheel.y);
	}
}

void
edit_render(void)
{
	gfx_begin();
	common_draw_map(current_layer, ui_slider_get_value(after_layer_alpha_slider));

	for(int x = 0; x < editor.map->w + 1; x++) {
		gfx_push_line(
			(vec2){ (x - 0.1), (0.0) }, 
			(vec2){ (x - 0.1), (editor.map->h) },
			0.05,
			(vec4){ 1.0, 1.0, 1.0, 1.0 }
		);
	}

	for(int y = 0; y < editor.map->h + 1; y++) {
		gfx_push_line(
			(vec2){ (0.0),           (y - 0.1) }, 
			(vec2){ (editor.map->w), (y - 0.1) },
			0.05,
			(vec4){ 1.0, 1.0, 1.0, 1.0 }
		);
	}
	if(!ui_is_active()) {
		draw_preview();
	}
	gfx_flush();
	gfx_end();
}

void
edit_enter(void)
{
	ui_child_append(ui_root(), tiles_root);
	ui_window_append_child(editor.controls_ui, cursor_layout);
	ui_window_append_child(editor.context_window, context_root);
	ui_window_append_child(editor.general_window, general_root);
}

void
edit_exit(void)
{
	ui_deparent(tiles_root);
	ui_deparent(cursor_layout);
	ui_deparent(context_root);
	ui_deparent(general_root);
}

void
edit_init(void)
{
	arrbuf_init(&fill_layer_helper);
	arrbuf_init(&fill_preview_stack);

	tiles_root = ui_new_object(0, UI_ROOT);
	context_root = ui_new_object(0, UI_ROOT);
	general_root = ui_new_object(0, UI_ROOT);
	
	context_layout = ui_layout_new();
	ui_layout_set_order(context_layout, UI_LAYOUT_VERTICAL);
	{
		UIObject *sublayout = ui_layout_new();
		ui_layout_set_order(sublayout, UI_LAYOUT_HORIZONTAL);
		ui_layout_set_border(sublayout, 2.5, 2.5, 2.5, 2.5);
		{
			UIObject *label = ui_label_new();
			ui_label_set_text(label, "Cursor Size: ");
			ui_label_set_alignment(label, UI_LABEL_ALIGN_RIGHT);
			ui_label_set_color(label, (vec4){ 1.0, 1.0, 1.0, 1.0 });
			ui_layout_append(sublayout, label);

			UIObject *slider_size = ui_slider_new();
			ui_slider_set_min_value(slider_size, 1.0);
			ui_slider_set_max_value(slider_size, 32.0);
			ui_slider_set_precision(slider_size, SLIDER_SIZE_PRECISION);
			ui_slider_set_value(slider_size, cursor_mode_size);
			ui_slider_set_callback(slider_size, NULL, cursor_size_cbk);

			ui_layout_append(sublayout, slider_size);

		}
		ui_layout_append(context_layout, sublayout);
	}
	ui_child_append(context_root, context_layout);

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

	cursor_layout = ui_layout_new();
	ui_layout_set_order(cursor_layout, UI_LAYOUT_HORIZONTAL);
	{
		for(int i = 0; i < LAST_CURSOR_MODE; i++) {
			cursor_checkboxes[i] = ui_checkbox_new();
			ui_checkbox_set_callback(cursor_checkboxes[i], &cursor_checkboxes[i], cursorchb_cbk);
			ui_checkbox_set_toggled(cursor_checkboxes[i], (CursorMode)i == cursor_mode);
			ui_layout_append(cursor_layout, cursor_checkboxes[i]);

			UIObject *img = ui_image_new();
			ui_image_set_keep_aspect(img, true);
			ui_image_set_stamp(img, &cursor_info[i].image);
			ui_layout_append(cursor_layout, img);
		}
	}

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
			ui_slider_set_max_value(slider_size, MAX_LAYERS - 1);
			ui_slider_set_precision(slider_size, MAX_LAYERS - 1);
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
	}
	ui_child_append(general_root, general_layout);

	(void)tileset_btn_cbk;
	ui_child_append(tiles_root, tileset_btn_window);
}

void
edit_terminate(void)
{
	ui_del_object(tiles_root);
	ui_del_object(cursor_layout);
	ui_del_object(general_root);

	arrbuf_free(&fill_layer_helper);
	arrbuf_free(&fill_preview_stack);
}


void
pencil_preview(int x, int y)
{
	vec2 v = { x + 0.5, y + 0.5 };
	int rows, cols;
	TextureStamp stamp;

	if(v[0] < 0 || v[0] >= editor.map->w || v[1] < 0 || v[1] >= editor.map->h) 
		return;

	gfx_sprite_count_rows_cols(SPRITE_TERRAIN, &rows, &cols);
	
	int tile = editor.current_tile - 1;
	stamp = get_sprite(SPRITE_TERRAIN, tile % cols, tile / cols);

	gfx_push_texture_rect(
			&stamp,
			v,
			(vec2){ 0.5, 0.5 },
			0.0,
			(vec4){ 1.0, 1.0, 1.0, 0.75 }
	);
}

void
draw_preview(void)
{
	if(cursors[cursor_mode].preview) {
		cursors[cursor_mode].preview(mouse_position[0], mouse_position[1]);
	}
}

void
apply_cursor(int x, int y)
{
	if(cursors[cursor_mode].apply) {
		cursors[cursor_mode].apply(x, y);
	}
}

void
pencil_apply(int x, int y)
{
	int x_min = x - (int)(cursor_mode_size) / 2;
	int x_max = x + (int)(cursor_mode_size) / 2;
	int y_min = y - (int)(cursor_mode_size) / 2;
	int y_max = y + (int)(cursor_mode_size) / 2;

	for(int xx = x_min; xx <= x_max; xx++)
	for(int yy = y_min; yy <= y_max; yy++) {
		if(xx < 0 || xx >= editor.map->w || yy < 0 || yy >= editor.map->h)
			continue;

		editor.map->tiles[xx + yy * editor.map->w + current_layer * editor.map->w * editor.map->h] = editor.current_tile;
	}
}

void
fill_apply(int x, int y)
{
	typedef struct {
		int x, y;
		int state;
	} StackElement;
	ArrayBuffer stack;
	StackElement *elem;
	int reference_tile;
	
	reference_tile = editor.map->tiles[x + y * editor.map->w + current_layer * editor.map->w * editor.map->h];
	if(reference_tile == editor.current_tile)
		return;

	arrbuf_init(&stack);
	arrbuf_insert(&stack, sizeof(StackElement), &(StackElement) {
		.x = x, .y = y, .state = 0
	});

	while((elem = arrbuf_peektop(&stack, sizeof(StackElement)))) {
		if(elem->x < 0 || elem->x >= editor.map->w || elem->y < 0 || elem->y >= editor.map->h) {
			arrbuf_poptop(&stack, sizeof(StackElement));
			continue;
		}
		int current_tile = editor.map->tiles[elem->x + elem->y * editor.map->w + current_layer * editor.map->w * editor.map->h];
		
		if(reference_tile != current_tile) {
			arrbuf_poptop(&stack, sizeof(StackElement));
			continue;
		}

		StackElement elems[] = {
			{
				.x = elem->x - 1,
				.y = elem->y,
				.state = 0
			},
			{
				.x = elem->x + 1,
				.y = elem->y,
				.state = 0
			},
			{
				.x = elem->x,
				.y = elem->y - 1,
				.state = 0
			},
			{
				.x = elem->x,
				.y = elem->y + 1,
				.state = 0
			}
		};
		editor.map->tiles[elem->x + elem->y * editor.map->w + current_layer * editor.map->w * editor.map->h] = editor.current_tile;
		/* elem dead here */
		arrbuf_poptop(&stack, sizeof(StackElement));
		arrbuf_insert(&stack, sizeof(elems), elems);
	}
	arrbuf_free(&stack);
}

void
cursorchb_cbk(UIObject *obj, void *userptr)
{
	(void)obj;
	UIObject **self = userptr;
	cursor_mode = self - cursor_checkboxes;
	for(int i = 0; i < LAST_CURSOR_MODE; i++) {
		ui_checkbox_set_toggled(cursor_checkboxes[i], &cursor_checkboxes[i] == self);
	}
}

void
cursor_size_cbk(UIObject *obj, void *userptr)
{
	(void)userptr;
	cursor_mode_size = ui_slider_get_value(obj);
}

void
layer_slider_cbk(UIObject *slider, void *ptr)
{
	(void)ptr;
	current_layer = ui_slider_get_value(slider);
}

void
tileset_btn_cbk(UIObject *obj, void *ptr)
{
	(void)ptr;
	obj = ui_get_parent(ui_get_parent(obj));

	if(ui_get_parent(tileset_window) == 0) {
		ui_child_append(tiles_root, tileset_window);
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
fill_preview(int x, int y)
{
	typedef struct {
		int x, y;
		int state;
	} StackElement;
	StackElement *elem;
	int reference_tile;

	int rows, cols;
	gfx_sprite_count_rows_cols(SPRITE_TERRAIN, &rows, &cols);

	arrbuf_clear(&fill_layer_helper);
	arrbuf_clear(&fill_preview_stack);

	int *map_info = arrbuf_newptr(&fill_layer_helper, editor.map->w * editor.map->h * sizeof(editor.map->tiles[0]));
	memcpy(map_info, &editor.map->tiles[current_layer * editor.map->w * editor.map->h], editor.map->w * editor.map->h * sizeof(editor.map->tiles[0]));

	if(x < 0 || y < 0 || x >= editor.map->w || y >= editor.map->h)
		return;

	reference_tile = map_info[x + y * editor.map->w];
	if(reference_tile == editor.current_tile)
		return;

	arrbuf_insert(&fill_preview_stack, sizeof(StackElement), &(StackElement) {
		.x = x, .y = y, .state = 0
	});
	int tile = editor.current_tile - 1;
	TextureStamp stamp = get_sprite(SPRITE_TERRAIN, tile % cols, tile / cols);

	Rectangle screen_rect = gfx_window_rectangle();
	/* grow to tile size, so we can use this as a rect-rect intersection in physics */
	screen_rect.half_size[0] += editor_get_zoom();
	screen_rect.half_size[1] += editor_get_zoom();
	
	while((elem = arrbuf_peektop(&fill_preview_stack, sizeof(StackElement)))) {
		if(elem->x < 0 || elem->x >= editor.map->w || elem->y < 0 || elem->y >= editor.map->h) {
			arrbuf_poptop(&fill_preview_stack, sizeof(StackElement));
			continue;
		}
		vec2 pixel_pos;
		gfx_world_to_pixel((vec2){ elem->x + 0.5, elem->y + 0.5 }, pixel_pos);
		/* see? */
		if(!rect_contains_point(&screen_rect, pixel_pos)) {
			arrbuf_poptop(&fill_preview_stack, sizeof(StackElement));
			continue;
		}
		
		int current_tile = map_info[elem->x + elem->y * editor.map->w];
		
		if(reference_tile != current_tile) {
			arrbuf_poptop(&fill_preview_stack, sizeof(StackElement));
			continue;
		}

		StackElement elems[] = {
			{
				.x = elem->x - 1,
				.y = elem->y,
				.state = 0
			},
			{
				.x = elem->x + 1,
				.y = elem->y,
				.state = 0
			},
			{
				.x = elem->x,
				.y = elem->y - 1,
				.state = 0
			},
			{
				.x = elem->x,
				.y = elem->y + 1,
				.state = 0
			}
		};
		map_info[elem->x + elem->y * editor.map->w] = editor.current_tile;
		gfx_push_texture_rect(&stamp, (vec2){ elem->x + 0.5, elem->y + 0.5 }, (vec2){ 0.5, 0.5 }, 0.0, (vec4){ 1.0, 1.0, 1.0, 0.5 });

		/* elem dead here */
		arrbuf_poptop(&fill_preview_stack, sizeof(StackElement));
		arrbuf_insert(&fill_preview_stack, sizeof(elems), elems);
	}
}

