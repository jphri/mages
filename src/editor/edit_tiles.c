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

#define SLIDER_SIZE_PRECISION 4

typedef enum CursorMode {
	CURSOR_MODE_PENCIL,
	CURSOR_MODE_FILL,
	LAST_CURSOR_MODE
} CursorMode;

typedef void (*CursorApply)(int x, int y);

static void apply_cursor(int x, int y);
static void pencil_apply(int x, int y);
static void fill_apply(int x, int y);

static CursorApply cursors[] = {
	[CURSOR_MODE_PENCIL] = pencil_apply,
	[CURSOR_MODE_FILL] = fill_apply
};

static struct {
	TextureStamp image;
} cursor_info[] = {
	[CURSOR_MODE_PENCIL] = { .image = { .texture = TEXTURE_UI, .position = { 0.0 / 256.0, 16.0 / 256.0 }, .size = { 8.0 / 256.0, 8.0 / 256.0 } } },
	[CURSOR_MODE_FILL]   = { .image = { .texture = TEXTURE_UI, .position = { 8.0 / 256.0, 16.0 / 256.0 }, .size = { 8.0 / 256.0, 8.0 / 256.0 } } },
};

static float zoom = 16.0;
static bool  ctrl_pressed = false;
static vec2  begin_offset, move_offset, offset;
static int current_layer = 0;
static MouseState mouse_state;
static vec2 mouse_position;

static UIObject controls_ui;
static CursorMode cursor_mode;
static float cursor_mode_size;

static UIObject cursor_checkboxes[LAST_CURSOR_MODE];

static void cursor_size_cbk(UIObject obj, void *userptr);
static void cursorchb_cbk(UIObject obj, void *userptr);
static void draw_cursor(void);

void
edit_keyboard(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_1: editor.editor_state = EDITOR_EDIT_MAP; break;
		case SDLK_2: editor.editor_state = EDITOR_SELECT_TILE; break;
		case SDLK_3: editor.editor_state = EDITOR_EDIT_COLLISION; break;
		case SDLK_d:
			if(cursor_mode == CURSOR_MODE_PENCIL)
				cursor_mode = CURSOR_MODE_FILL;
			else
				cursor_mode = CURSOR_MODE_PENCIL;
			break;
		case SDLK_LCTRL: ctrl_pressed = true; break;
		case SDLK_s:
			if(ctrl_pressed) {
				export_map("exported_map.map");
				printf("Map exported to exported_map.map\n");
			}
			break;
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
	int x, y;
	mouse_position[0] = event->motion.x;
	mouse_position[1] = event->motion.y;

	switch(mouse_state) {
	case MOUSE_MOVING:
		offset[0] = begin_offset[0] + event->motion.x - move_offset[0];
		offset[1] = begin_offset[1] + event->motion.y - move_offset[1];
		break;
	case MOUSE_DRAWING:
		x = ((event->motion.x - offset[0] - 0.5) / zoom);
		y = ((event->motion.y - offset[1] - 0.5) / zoom);
		apply_cursor(x, y);
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
	mouse_position[0] = event->motion.x;
	mouse_position[1] = event->motion.y;

	
	if(event->type == SDL_MOUSEBUTTONDOWN && mouse_state == MOUSE_NOTHING) {
		switch(event->button.button) {
		case SDL_BUTTON_LEFT: 
			fake_event.type = SDL_MOUSEMOTION;
			fake_event.motion.x = mouse_position[0];
			fake_event.motion.y = mouse_position[1];
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
		zoom += event->wheel.y;
		if(zoom < 1.0)
			zoom = 1.0;
	} else {
		current_layer += event->wheel.y;
		if(current_layer < 0)
			current_layer = 0;
		if(current_layer > SCENE_LAYERS)
			current_layer = SCENE_LAYERS;

		printf("Current Layer: %d\n", current_layer);
	}
}

void
edit_render(void)
{
	TextureStamp stamp;
	gfx_set_camera(offset, (vec2){ zoom, zoom });


	gfx_draw_begin(NULL);

	for(int k = 0; k < SCENE_LAYERS; k++)
	for(int i = 0; i < editor.map->w * editor.map->h; i++) {
		float x = (     (i % editor.map->w) + 0.5);
		float y = ((int)(i / editor.map->w) + 0.5);

		int spr = editor.map->tiles[i + k * editor.map->w * editor.map->h] - 1;
		if(spr < 0)
			continue;
		int spr_x = spr % 16;
		int spr_y = spr / 16;

		stamp = get_sprite(SPRITE_TERRAIN, spr_x, spr_y);

		gfx_draw_texture_rect(
				&stamp,
				(vec2){ x, y },
				(vec2){ 0.5, 0.5 },
				0.0,
				(vec4){ 1.0, 1.0, 1.0, 1.0 }
		);
	}

	for(int x = 0; x < editor.map->w + 1; x++) {
		gfx_draw_line(
			(vec2){ (x - 0.1), (0.0) }, 
			(vec2){ (x - 0.1), (editor.map->h) },
			0.05,
			(vec4){ 1.0, 1.0, 1.0, 1.0 }
		);
	}
	for(int y = 0; y < editor.map->h + 1; y++) {
		gfx_draw_line(
			(vec2){ (0.0),           (y - 0.1) }, 
			(vec2){ (editor.map->w), (y - 0.1) },
			0.05,
			(vec4){ 1.0, 1.0, 1.0, 1.0 }
		);
	}
	if(!ui_is_active()) {
		draw_cursor();
	}
	gfx_draw_end();
}

void
edit_enter(void)
{

	UIObject layout = ui_layout_new();

	UIObject cursor_layout = ui_layout_new();
	ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
	ui_layout_set_order(cursor_layout, UI_LAYOUT_HORIZONTAL);

	for(int i = 0; i < LAST_CURSOR_MODE; i++) {
		cursor_checkboxes[i] = ui_checkbox_new();
		ui_checkbox_set_callback(cursor_checkboxes[i], &cursor_checkboxes[i], cursorchb_cbk);
		ui_checkbox_set_toggled(cursor_checkboxes[i], (CursorMode)i == cursor_mode);
		ui_layout_append(cursor_layout, cursor_checkboxes[i]);

		UIObject img = ui_image_new();
		ui_image_set_keep_aspect(img, true);
		ui_image_set_stamp(img, &cursor_info[i].image);
		ui_layout_append(cursor_layout, img);
	}
	ui_layout_append(layout, cursor_layout);

	UIObject slider_size = ui_slider_new();
	ui_slider_set_max_value(slider_size, 31 * SLIDER_SIZE_PRECISION);
	ui_slider_set_value(slider_size, cursor_mode_size - 1);
	ui_slider_set_callback(slider_size, NULL, cursor_size_cbk);
	ui_layout_append(layout, slider_size);

	controls_ui = ui_window_new();
	ui_window_set_size(controls_ui, (vec2){ 150, 150 });
	ui_window_set_position(controls_ui, (vec2){ 0 + 150, 600 - 150 });
	ui_window_set_decorated(controls_ui, false);
	ui_window_set_child(controls_ui, layout);

	ui_map(controls_ui);
}

void
edit_exit(void)
{
	ui_del_object(controls_ui);
}

void
draw_cursor(void)
{
	vec2 v;

	vec2_sub(v, mouse_position, offset);
	vec2_sub(v, v, (vec2){ 0.5, 0.5 });
	vec2_div(v, v, (vec2){ zoom, zoom });
	vec2_floor(v, v);
	vec2_add(v, v, (vec2){ 0.5, 0.5 });

	if(v[0] < 0 || v[0] >= editor.map->w || v[1] < 0 || v[1] >= editor.map->h) 
		return;

	gfx_draw_rect(v, (vec2){ 0.5 , 0.5 }, 0.05, (vec4){ 1.0, 1.0, 1.0, 1.0 });
}

void
apply_cursor(int x, int y)
{
	if(cursors[cursor_mode])
		cursors[cursor_mode](x, y);
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
cursorchb_cbk(UIObject obj, void *userptr)
{
	(void)obj;
	UIObject *self = userptr;
	cursor_mode = self - cursor_checkboxes;
	for(int i = 0; i < LAST_CURSOR_MODE; i++) {
		ui_checkbox_set_toggled(cursor_checkboxes[i], &cursor_checkboxes[i] == self);
	}
}

void
cursor_size_cbk(UIObject obj, void *userptr)
{
	(void)userptr;
	cursor_mode_size = ui_slider_get_value(obj) / (float)SLIDER_SIZE_PRECISION + 1;
	printf("cursor_mode_size: %f\n", cursor_mode_size);
}
