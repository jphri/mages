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
#include "SDL_keycode.h"
#include "editor.h"

#define MAX_LAYERS 64

typedef struct MapBrush MapBrush;

typedef struct {
	void (*begin)(int x, int y);
	void (*drag)(int x, int y);
	void (*end)(int x, int y);
	void (*draw)(void);
} Cursor;

struct MapBrush{
	int tile;
	int collidable;
	vec2 position, half_size;

	MapBrush *next, *prev;
};

static void layer_slider_cbk(UIObject *obj, void *userptr);

static void rect_begin(int x, int y);
static void rect_drag(int x, int y);
static void rect_end(int x, int y);
static void rect_draw(void);

static void select_begin(int x, int y);
static void select_end(int x, int y);
static void select_drag(int x, int y);

static void insert_brush_before(MapBrush *node, MapBrush *bef);
static void insert_brush_after(MapBrush *node, MapBrush *aft);
static void remove_brush(MapBrush *node);

static void integer_round_cbk(UIObject *obj, void *userptr);

static bool  ctrl_pressed = false;
static vec2  begin_offset, move_offset, offset;
static int current_layer = 0;
static MouseState mouse_state;
static MapBrush current_collision;

static UIObject *general_root;
static UIObject *after_layer_alpha_slider;
static UIObject *integer_round;

static MapBrush *brush_list, *end_list, *selected_brush;

void
collision_init(void)
{
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

			BEGIN_SUB("Integer round:") {
				integer_round = ui_checkbox_new();
				ui_checkbox_set_toggled(integer_round, false);
				ui_checkbox_set_callback(integer_round, NULL, integer_round_cbk);

				ui_layout_append(sublayout, integer_round);
			} END_SUB;
		}
		ui_child_append(general_root, general_layout);
	}

	#undef BEGIN_SUB
	#undef END_SUB
	selected_brush = NULL;
}

void
collision_terminate(void)
{
	ui_del_object(general_root);
}

void
collision_enter(void)
{
	ui_window_append_child(editor.general_window, general_root);
}

void
collision_exit(void)
{
	ui_deparent(general_root);
}

void
collision_keyboard(SDL_Event *event)
{
	MapBrush *current_next, *current_prev;

	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_LCTRL: ctrl_pressed = true; break;
		case SDLK_UP:
			if(!selected_brush)
				break;

			if(selected_brush == end_list)
				break;
			
			if(ctrl_pressed) {
				selected_brush = selected_brush->next;
				break;
			}

			current_next = selected_brush->next;
			remove_brush(selected_brush);
			insert_brush_after(selected_brush, current_next);
			break;
		case SDLK_DELETE:
			if(!selected_brush)
				break;
			remove_brush(selected_brush);
			free(selected_brush);
			selected_brush = NULL;
			break;
		case SDLK_DOWN:
			if(!selected_brush)
				break;

			if(selected_brush == brush_list)
				break;

			if(ctrl_pressed) {
				selected_brush = selected_brush->prev;
				break;
			}

			current_prev = selected_brush->prev;
			remove_brush(selected_brush);
			insert_brush_before(selected_brush, current_prev);
			break;
		}
		
	} else {
		switch(event->key.keysym.sym) {
		case SDLK_LCTRL: ctrl_pressed = false; break;
		}
	}
}

void
collision_mouse_motion(SDL_Event *event)
{
	vec2 v;

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
		select_drag(event->motion.x, event->motion.y);
		break;
	default:
		do {} while(0);
	}
}

void
collision_mouse_button(SDL_Event *event)
{
	vec2 pos;
	if(event->type == SDL_MOUSEBUTTONUP) {
		switch(mouse_state) {
		case MOUSE_DRAWING: rect_end(event->button.x, event->button.y); break;
		case MOUSE_MOVING_BRUSH: select_end(event->button.x, event->button.y); break;
		default:
			break;
		}
		mouse_state = MOUSE_NOTHING;
	}
	
	if(event->type == SDL_MOUSEBUTTONDOWN && mouse_state == MOUSE_NOTHING) {
		switch(event->button.button) {
		case SDL_BUTTON_RIGHT: 
			rect_begin(event->button.x, event->button.y);
			mouse_state = MOUSE_DRAWING; 
			break;
		case SDL_BUTTON_MIDDLE: 
			move_offset[0] = event->button.x; 
			move_offset[1] = event->button.y; 
			vec2_dup(begin_offset, offset);
			mouse_state = MOUSE_MOVING; 
			break;
		case SDL_BUTTON_LEFT:
			gfx_pixel_to_world((vec2){ event->button.x, event->button.y }, pos);
			select_begin(event->motion.x, event->motion.y);
			mouse_state = MOUSE_MOVING_BRUSH;
			break;
		}
	}
}

void
collision_wheel(SDL_Event *event)
{
	if(ctrl_pressed) {
		editor_delta_zoom(event->wheel.y);
	}
}

void
collision_render(void)
{
	gfx_begin();
	//common_draw_map(current_layer, ui_slider_get_value(after_layer_alpha_slider));

	int rows, cols;
	gfx_sprite_count_rows_cols(SPRITE_TERRAIN, &rows, &cols);

	for(MapBrush *brush = brush_list; brush; brush = brush->next) {
		int tile = brush->tile - 1;
		TextureStamp stamp = get_sprite(SPRITE_TERRAIN, tile % cols, tile / cols);
		vec4 color = { 1.0, 1.0, 1.0, 1.0 };

		if(selected_brush && selected_brush != brush) {
			Rectangle test_rect;
			vec2_dup(test_rect.position, brush->position);
			vec2_add(test_rect.half_size, brush->half_size, selected_brush->half_size);

			if(rect_contains_point(&test_rect, selected_brush->position)) {
				color[3] = 0.25;
			}
		}

		if(selected_brush == brush) {
			color[1] = 0.0;
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

	if(mouse_state == MOUSE_DRAWING) {
		rect_draw();
	}
	gfx_flush();
	gfx_end();
}

void
layer_slider_cbk(UIObject *slider, void *userptr)
{
	(void)userptr;
	current_layer = ui_slider_get_value(slider);
}


void
rect_begin(int x, int y)
{
	gfx_pixel_to_world((vec2){ x, y }, begin_offset);
	if(ui_checkbox_get_toggled(integer_round))
		vec2_round(begin_offset, begin_offset);

	if(!selected_brush) {
		selected_brush = malloc(sizeof(MapBrush));
		selected_brush->half_size[0] = 0;
		selected_brush->half_size[1] = 0;
		selected_brush->tile = editor.current_tile;
		selected_brush->next = NULL;
		selected_brush->prev = NULL;
		vec2_dup(selected_brush->position, begin_offset);

		if(brush_list)
			insert_brush_after(selected_brush, end_list);
		else {
			brush_list = selected_brush;
			end_list = selected_brush;
		}
	}
}

void
rect_drag(int x, int y)
{
	vec2 delta;
	vec2 v;

	gfx_pixel_to_world((vec2){ x, y }, v);
	if(ui_checkbox_get_toggled(integer_round))
		vec2_round(v, v);

	vec2_sub(delta, v, begin_offset);
	vec2_add_scaled(selected_brush->position, selected_brush->position, delta, 0.5);
	vec2_add_scaled(selected_brush->half_size, selected_brush->half_size, delta, 0.5);

	vec2_dup(begin_offset, v);

}

void
rect_end(int x, int y)
{
	(void)x;
	(void)y;

	selected_brush->half_size[0] = fabsf(selected_brush->half_size[0]);
	selected_brush->half_size[1] = fabsf(selected_brush->half_size[1]);

	if(selected_brush->half_size[0] < 1.0 / 64.0 || selected_brush->half_size[1] < 1.0 / 64.0) {
		remove_brush(selected_brush);
		free(selected_brush);
		selected_brush = NULL;
	}
}

void
rect_draw(void)
{
	int rows, cols, tile;
	gfx_sprite_count_rows_cols(SPRITE_TERRAIN, &rows, &cols);

	tile = current_collision.tile - 1;
	TextureStamp stamp = get_sprite(SPRITE_TERRAIN, tile % cols, tile / cols);
	gfx_push_texture_rect(
		&stamp, 
		current_collision.position, 
		current_collision.half_size, 
		(vec2){ current_collision.half_size[0] * 2.0, current_collision.half_size[1] * 2.0 }, 
		0.0, 
		(vec4){ 1.0, 1.0, 1.0, 1.0 });
		gfx_push_rect(current_collision.position, current_collision.half_size, 0.5/(editor_get_zoom()), (vec4){ 1.0, 1.0, 1.0, 1.0 });
	
}

void
integer_round_cbk(UIObject *obj, void *userptr)
{
	(void)userptr;
	ui_checkbox_set_toggled(obj, !ui_checkbox_get_toggled(obj));
}

void
select_begin(int x, int y)
{
	vec2 p;
	gfx_pixel_to_world((vec2){ x, y }, p);

	selected_brush = NULL;
	for(MapBrush *b = end_list; b; b = b->prev) {
		Rectangle rect = {
			.position = { b->position[0], b->position[1] },
			.half_size = { b->half_size[0], b->half_size[1] }
		};

		if(rect_contains_point(&rect, p)) {
			selected_brush = b;
			break;
		}
	}
	if(!selected_brush)
		return;

	gfx_pixel_to_world((vec2){ x, y }, begin_offset);
	if(ui_checkbox_get_toggled(integer_round))
		vec2_round(begin_offset, begin_offset);
}

void
select_drag(int x, int y)
{
	if(!selected_brush)
		return;

	vec2 v, p;
	gfx_pixel_to_world((vec2){ x, y }, v);
	if(ui_checkbox_get_toggled(integer_round))
		vec2_round(v, v);

	vec2_sub(p, begin_offset, v);
	vec2_dup(begin_offset, v);
	vec2_sub(selected_brush->position, selected_brush->position, p);
	
}

void
select_end(int x, int y)
{
	(void)x;
	(void)y;
}

void
insert_brush_after(MapBrush *b, MapBrush *aft)
{
	b->next = aft->next;
	if(aft->next)
		aft->next->prev = b;
	else
		end_list = b;

	b->prev = aft;
	aft->next = b;
}

void
insert_brush_before(MapBrush *b, MapBrush *bef)
{
	b->prev = bef->prev;
	if(bef->prev)
		bef->prev->next = b;
	else
		brush_list = b;
	b->next = bef;
	bef->prev = b;
}

void
remove_brush(MapBrush *brush)
{
	if(brush->prev)
		brush->prev->next = brush->next;
	else
	 	brush_list = brush->next;

	if(brush->next)
		brush->next->prev = brush->prev;
	else
	 	end_list = brush->prev;
}
