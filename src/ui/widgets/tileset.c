#include "ui.h"

#define SPRITE_SIZE 32

#define TSET(OBJ) WIDGET(UI_TILESET_SEL, OBJ)

static void process_scrolls(UIObject *tileset, UIEvent *ev, Rectangle *rect);
static void draw_tileset(UIObject *tileset, Rectangle *rect);
static void tileset_button(UIObject *tileset, UIEvent *ev, Rectangle *rect);

static void vscroll_cbk(UIObject *vscroll, void *ptr);
static void hscroll_cbk(UIObject *hscroll, void *ptr);

UIObject *
ui_tileset_sel_new(void)
{
	UIObject *tileset =  ui_new_object(0, UI_TILESET_SEL);
	UIObject *hscroll = ui_slider_new();
	UIObject *vscroll = ui_slider_new();

	ui_slider_enable_label(hscroll, false);
	ui_slider_enable_label(vscroll, false);
	ui_slider_set_vertical(vscroll, true);

	ui_slider_set_callback(vscroll, NULL, vscroll_cbk);
	ui_slider_set_callback(hscroll, NULL, hscroll_cbk);

	ui_child_append(tileset, hscroll);
	ui_child_append(tileset, vscroll);

	gfx_sprite_count_rows_cols(SPRITE_TERRAIN, &TSET(tileset)->rows, &TSET(tileset)->cols);
	TSET(tileset)->cbk = NULL;
	TSET(tileset)->hscroll = hscroll;
	TSET(tileset)->vscroll = vscroll;

	return tileset;
}

void
ui_tileset_sel_set_cbk(UIObject *tilesel, void *userptr, void (*cbk)(UIObject *obj, void *userptr))
{
	TSET(tilesel)->userptr = userptr;
	TSET(tilesel)->cbk = cbk;
}

int
ui_tileset_sel_get_selected(UIObject *tilesel)
{
	return TSET(tilesel)->selected;
}

void
UI_TILESET_SEL_event(UIObject *obj, UIEvent *ev, Rectangle *r)
{
	(void)obj;
	ui_default_mouse_handle(obj, ev, r);

	switch(ev->event_type) {
	case UI_DRAW:
		draw_tileset(obj, r);
		process_scrolls(obj, ev, r);
		break;
	case UI_MOUSE_BUTTON:
		process_scrolls(obj, ev, r);
		tileset_button(obj, ev, r);
		break;
	case UI_MOUSE_MOTION:
		process_scrolls(obj, ev, r);
		break;
	default:
		break;
	}
}

void
draw_tileset(UIObject *obj, Rectangle *rect)
{
	vec2 line_min, line_max;
	rect_boundaries(line_min, line_max, rect);

	vec2_sub(line_min, line_min, TSET(obj)->offset);
	line_max[0] = line_min[0] + TSET(obj)->rows * SPRITE_SIZE;
	line_max[1] = line_min[1] + TSET(obj)->cols * SPRITE_SIZE;

	for(int i = 0; i < TSET(obj)->rows * TSET(obj)->cols; i++) {
		float x = (     (i % TSET(obj)->cols) + 0.5) * SPRITE_SIZE + line_min[0];
		float y = ((int)(i / TSET(obj)->cols) + 0.5) * SPRITE_SIZE + line_min[1];

		int spr = i - 1;
		int spr_x = spr % TSET(obj)->cols;
		int spr_y = spr / TSET(obj)->cols;

		TextureStamp sprite = {
			.texture = TERRAIN_NORMAL,
			.position = {
				spr_x / (float)TSET(obj)->cols,
				spr_y / (float)TSET(obj)->rows,
			},
			.size = {
				1.0 / (float)TSET(obj)->cols,
				1.0 / (float)TSET(obj)->rows,
			}
		};
		gfx_push_texture_rect(&sprite, (vec2){ x, y }, (vec2){ SPRITE_SIZE / 2.0, SPRITE_SIZE / 2.0 }, (vec2){ 1.0, 1.0 }, 0.0, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	}

	for(int x = 0; x <= TSET(obj)->cols; x++) {
		gfx_push_line(
				(vec2){ line_min[0] + x * SPRITE_SIZE, line_min[1] }, 
				(vec2){ line_min[0] + x * SPRITE_SIZE, line_max[1] },
				1.0, 
				(vec4){ 1.0, 1.0, 1.0, 1.0 }
				);
	}

	for(int y = 0; y <= TSET(obj)->rows; y++) {
		gfx_push_line(
				(vec2){ line_min[0], line_min[1] + y * SPRITE_SIZE }, 
				(vec2){ line_max[0], line_min[1] + y * SPRITE_SIZE },
				1.0, 
				(vec4){ 1.0, 1.0, 1.0, 1.0 }
				);
	}
}


void 
tileset_button(UIObject *obj, UIEvent *ev, Rectangle *rect)
{
	vec2 line_min, line_max;
	Rectangle r;
	rect_boundaries(line_min, line_max, rect);

	vec2_sub(line_min, line_min, TSET(obj)->offset);
	line_max[0] = line_min[0] + TSET(obj)->rows * SPRITE_SIZE;
	line_max[1] = line_min[1] + TSET(obj)->cols * SPRITE_SIZE;

	r = rect_from_boundaries(line_min, line_max);
	
	if(ui_get_hot() == obj && ev->data.mouse.button == UI_MOUSE_LEFT && ev->data.mouse.state) {
		vec2 p;
		if(!rect_contains_point(&r, ev->data.mouse.position))
			return;

		vec2_sub(p, ev->data.mouse.position, line_min);
		vec2_div(p, p, (vec2){ SPRITE_SIZE, SPRITE_SIZE });
		vec2_floor(p, p);

		int index = p[0] + p[1] * TSET(obj)->cols;
		TSET(obj)->selected = index;
		if(TSET(obj)->cbk)
			TSET(obj)->cbk(obj, TSET(obj)->userptr);
	}
}

void
process_scrolls(UIObject *obj, UIEvent *ev, Rectangle *r)
{
	vec2 line_min, line_max;
	rect_boundaries(line_min, line_max, r);
	
	line_max[0] = line_min[0] + TSET(obj)->rows * SPRITE_SIZE;
	line_max[1] = line_min[1] + TSET(obj)->cols * SPRITE_SIZE ;

	Rectangle total = rect_from_boundaries(line_min, line_max);

	if(total.position[1] > r->position[1]) {
		float handle_size = r->half_size[1] * r->half_size[1] / total.half_size[1];
		float delta = (total.position[1] - r->position[1]) * 2.0;

		ui_slider_set_min_value(TSET(obj)->vscroll, 0.0);
		ui_slider_set_max_value(TSET(obj)->vscroll, delta + 10);
		ui_slider_set_precision(TSET(obj)->vscroll, delta + 10);
		ui_slider_set_handle_size(TSET(obj)->vscroll, handle_size);

		ui_call_event(TSET(obj)->vscroll, ev, &(Rectangle){
			.position = { r->position[0] + r->half_size[0] - 5.0, r->position[1] },
			.half_size = { 5.0, r->half_size[1] }
		});
	}

	if(total.position[0] > r->position[0]) {
		float rect_offset = total.position[1] > r->position[1] ? 5.0 : 0.0;
		float handle_size = (r->half_size[1] - rect_offset) * r->half_size[1] / total.half_size[1];
		float delta = (total.position[0] - r->position[0]) * 2.0;

		ui_slider_set_min_value(TSET(obj)->hscroll, 0.0);
		ui_slider_set_max_value(TSET(obj)->hscroll, delta + 10);
		ui_slider_set_precision(TSET(obj)->hscroll, delta + 10);
		ui_slider_set_handle_size(TSET(obj)->hscroll, handle_size);

		ui_call_event(TSET(obj)->hscroll, ev, &(Rectangle){
			.position = { r->position[0] - rect_offset, r->position[1] + r->half_size[1] - 5.0 },
			.half_size = { r->half_size[0] - rect_offset, 5.0 }
		});
	}
}

void 
vscroll_cbk(UIObject *vscroll, void *ptr)
{
	(void)ptr;

	UIObject *tset = ui_get_parent(vscroll);
	TSET(tset)->offset[1] = ui_slider_get_value(vscroll);
}

void hscroll_cbk(UIObject *hscroll, void *ptr)
{
	(void)ptr;

	UIObject *tset = ui_get_parent(hscroll);
	TSET(tset)->offset[0] = ui_slider_get_value(hscroll);
}
