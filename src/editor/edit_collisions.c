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
#include "editor.h"

#define MAX_LAYERS 64

typedef enum CursorMode {
	CURSOR_RECTANGLE,
	CURSOR_FILL,
	LAST_CURSOR_MODE
} CursorMode;

typedef struct {
	void (*begin)(int x, int y);
	void (*drag)(int x, int y);
	void (*end)(int x, int y);
	void (*draw)(void);
} Cursor;

static void cursor_begin(int x, int y);
static void cursor_drag(int x, int y);
static void cursor_end(int x, int y);
static void cursor_draw(void);

static void layer_slider_cbk(UIObject obj, void *userptr);
static void cursor_cbk(UIObject obj, void *userptr);

static void rect_begin(int x, int y);
static void rect_drag(int x, int y);
static void rect_end(int x, int y);
static void rect_draw(void);

static void fill_end(int x, int y);

static int *fill_find_elements_from(int x, int y);
static void fill_find_rect(int x, int y, int *tileset);
static void fill_new_rect(int x, int y, int w, int h);

static Cursor cursor[] = {
	[CURSOR_RECTANGLE] = {
		.begin = rect_begin,
		.drag = rect_drag,
		.end = rect_end,
		.draw = rect_draw
	},
	[CURSOR_FILL] = {
		.end = fill_end
	}
};

static struct {
	TextureStamp image;
} cursor_info[] = {
	[CURSOR_RECTANGLE] = { .image = { .texture = TEXTURE_UI, .position = { 3 * 8.0 / 256.0, 0.0 / 256.0 }, .size = { 8.0 / 256.0, 8.0 / 256.0 } } },
	[CURSOR_FILL] = { .image = { .texture = TEXTURE_UI, .position = { 1 * 8.0 / 256.0, 2 * 8.0 / 256.0 }, .size = { 8.0 / 256.0, 8.0 / 256.0 } } },
};

static bool  ctrl_pressed = false;
static vec2  begin_offset, move_offset, offset;
static int current_layer = 0;
static MouseState mouse_state;
static CollisionData current_collision;
static CursorMode current_cursor;

static ArrayBuffer fill_stack;
static ArrayBuffer fill_layer_helper;

static UIObject general_root;
static UIObject after_layer_alpha_slider;
static UIObject cursors_ui;

static UIObject cursor_checkboxes[LAST_CURSOR_MODE];

void
collision_init(void)
{
	arrbuf_init(&fill_stack);
	arrbuf_init(&fill_layer_helper);

	general_root = ui_new_object(0, UI_ROOT);
	{
		UIObject general_layout = ui_layout_new();
		ui_layout_set_order(general_layout, UI_LAYOUT_VERTICAL);
		{
			UIObject sublayout, label;

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
				UIObject slider_size = ui_slider_new();
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
	}

	#undef BEGIN_SUB
	#undef END_SUB

	cursors_ui = ui_layout_new();
	ui_layout_set_order(cursors_ui, UI_LAYOUT_HORIZONTAL);
	{
		for(int i = 0; i < LAST_CURSOR_MODE; i++) {
			cursor_checkboxes[i] = ui_checkbox_new();
			ui_checkbox_set_callback(cursor_checkboxes[i], &cursor_checkboxes[i], cursor_cbk);
			ui_checkbox_set_toggled(cursor_checkboxes[i], (CursorMode)i == current_cursor);
			ui_layout_append(cursors_ui, cursor_checkboxes[i]);

			UIObject img = ui_image_new();
			ui_image_set_keep_aspect(img, true);
			ui_image_set_stamp(img, &cursor_info[i].image);
			ui_layout_append(cursors_ui, img);
		}
	}
}

void
collision_terminate(void)
{
	ui_del_object(general_root);
	ui_del_object(cursors_ui);
}

void
collision_enter(void)
{
	ui_window_append_child(editor.general_window, general_root);
	ui_window_append_child(editor.controls_ui, cursors_ui);
}

void
collision_exit(void)
{
	ui_deparent(general_root);
	ui_deparent(cursors_ui);
}

void
collision_keyboard(SDL_Event *event)
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
		cursor_drag(event->motion.x, event->motion.y);
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
		if(mouse_state == MOUSE_DRAWING) {
			cursor_end(event->button.x, event->button.y);
		}
		mouse_state = MOUSE_NOTHING;
	}
	
	if(event->type == SDL_MOUSEBUTTONDOWN && mouse_state == MOUSE_NOTHING) {
		switch(event->button.button) {
		case SDL_BUTTON_LEFT: 
			cursor_begin(event->button.x, event->button.y);
			mouse_state = MOUSE_DRAWING; 
			break;
		case SDL_BUTTON_RIGHT: 
			move_offset[0] = event->button.x; 
			move_offset[1] = event->button.y; 
			vec2_dup(begin_offset, offset);
			mouse_state = MOUSE_MOVING; 
			break;
		case SDL_BUTTON_MIDDLE:
			gfx_pixel_to_world((vec2){ event->button.x, event->button.y }, pos);
			
			for(CollisionData *c = editor.map->collision, *bef = NULL; c; bef = c, c = c->next) {
				vec2 min, max;

				vec2_add(max, c->position, c->half_size);
				vec2_sub(min, c->position, c->half_size);
				
				if(pos[0] < min[0] || pos[0] > max[0] || pos[1] < min[1] || pos[1] > max[1])
					continue;

				if(bef) 
					bef->next = c->next;
				if(editor.map->collision == c)
					editor.map->collision = c->next;
				
				break;
			}
			break;
		}
	}
}

void
collision_wheel(SDL_Event *event)
{
	if(ctrl_pressed) {
		editor_delta_zoom(event->wheel.y);
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
collision_render(void)
{
	TextureStamp stamp;

	//gfx_debug_begin();
	//for(CollisionData *c = editor.map->collision; c; c = c->next) {
	//	vec2_dup(pos, c->position);
	//	gfx_debug_quad(pos, c->half_size);
	//}

	//if(mouse_state == MOUSE_DRAWING) {
	//	gfx_debug_set_color((vec4){ 1.0, 1.0, 1.0, 1.0 });
	//	gfx_debug_quad(current_collision.position, current_collision.half_size);
	//}
	//gfx_debug_end();

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
		gfx_draw_texture_rect(&stamp, (vec2){ x, y }, (vec2){ 0.5, 0.5 }, 0.0, (vec4){ 1.0, 1.0, 1.0, k > current_layer ? ui_slider_get_value(after_layer_alpha_slider) : 1.0 });
	}

	for(CollisionData *c = editor.map->collision; c; c = c->next) {
		gfx_draw_rect(c->position, c->half_size, 0.15, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	}

	if(mouse_state == MOUSE_DRAWING) {
		cursor_draw();
	}
	gfx_draw_end();
}

void
cursor_begin(int x, int y)
{
	if(cursor[current_cursor].begin)
		cursor[current_cursor].begin(x, y);
}

void
cursor_drag(int x, int y)
{
	if(cursor[current_cursor].drag)
		cursor[current_cursor].drag(x, y);
}

void
cursor_end(int x, int y)
{
	if(cursor[current_cursor].end)
		cursor[current_cursor].end(x, y);
}

void
cursor_draw(void)
{
	if(cursor[current_cursor].draw)
		cursor[current_cursor].draw();
}

void
layer_slider_cbk(UIObject slider, void *userptr)
{
	(void)userptr;
	current_layer = ui_slider_get_value(slider);
}

void 
cursor_cbk(UIObject obj, void *userptr)
{
	(void)obj;
	UIObject *self = userptr;
	current_cursor = self - cursor_checkboxes;
	for(int i = 0; i < LAST_CURSOR_MODE; i++) {
		ui_checkbox_set_toggled(cursor_checkboxes[i], &cursor_checkboxes[i] == self);
	}
}

void
rect_begin(int x, int y)
{
	gfx_pixel_to_world((vec2){ x, y }, begin_offset);
}

void
rect_drag(int x, int y)
{
	vec2 full_size, begin_position;
	vec2 v;

	gfx_pixel_to_world((vec2){ x, y }, v);

	vec2_sub(full_size, v, begin_offset);
	vec2_div(current_collision.half_size, full_size, (vec2){ 2.0, 2.0 });
	current_collision.half_size[0] = fabs(current_collision.half_size[0]);
	current_collision.half_size[1] = fabs(current_collision.half_size[1]);

	if(full_size[0] > 0)
		begin_position[0] = begin_offset[0];
	else
		begin_position[0] = begin_offset[0] + full_size[0];

	if(full_size[1] > 0)
		begin_position[1] = begin_offset[1];
	else
		begin_position[1] = begin_offset[1] + full_size[1];

	vec2_add(current_collision.position, begin_position, current_collision.half_size);
}

void
rect_end(int x, int y)
{
	(void)x;
	(void)y;

	CollisionData *data = malloc(sizeof(*data));
	*data = current_collision;
	data->next = editor.map->collision;
	editor.map->collision = data;

	printf("new collision at %f %f %f %f\n",
			data->position[0],
			data->position[1],
			data->half_size[0],
			data->half_size[1]);
}

void
rect_draw(void)
{
	gfx_draw_rect(current_collision.position, current_collision.half_size, 0.15, (vec4){ 1.0, 1.0, 1.0, 1.0 });
}

void
fill_end(int x, int y)
{
	vec2 v;
	gfx_pixel_to_world((vec2){ x, y }, v);
	printf("%f %f\n", v[0], v[1]);

	int *tileset = fill_find_elements_from(v[0], v[1]);
	if(!tileset)
		return;

	for(int y = 0; y < editor.map->h; y++)
	for(int x = 0; x < editor.map->w; x++) {
		if(tileset[x + y * editor.map->w] < 0)
			fill_find_rect(x, y, tileset);
	}
}

int *
fill_find_elements_from(int x, int y)
{
	typedef struct {
		int x, y;
		int state;
	} StackElement;
	StackElement *elem;
	int reference_tile;

	arrbuf_clear(&fill_layer_helper);
	arrbuf_clear(&fill_stack);

	int *map_info = arrbuf_newptr(&fill_layer_helper, editor.map->w * editor.map->h * sizeof(editor.map->tiles[0]));
	memcpy(map_info, &editor.map->tiles[current_layer * editor.map->w * editor.map->h], editor.map->w * editor.map->h * sizeof(editor.map->tiles[0]));

	if(x < 0 || y < 0 || x >= editor.map->w || y >= editor.map->h)
		return NULL;

	reference_tile = map_info[x + y * editor.map->w];

	arrbuf_insert(&fill_stack, sizeof(StackElement), &(StackElement) {
		.x = x, .y = y, .state = 0
	});

	while((elem = arrbuf_peektop(&fill_stack, sizeof(StackElement)))) {
		if(elem->x < 0 || elem->x >= editor.map->w || elem->y < 0 || elem->y >= editor.map->h) {
			arrbuf_poptop(&fill_stack, sizeof(StackElement));
			continue;
		}
		int current_tile = map_info[elem->x + elem->y * editor.map->w];
		
		if(reference_tile != current_tile) {
			arrbuf_poptop(&fill_stack, sizeof(StackElement));
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
		map_info[elem->x + elem->y * editor.map->w] = -1;

		/* elem dead here */
		arrbuf_poptop(&fill_stack, sizeof(StackElement));
		arrbuf_insert(&fill_stack, sizeof(elems), elems);
	}

	return map_info;
}

void
fill_find_rect(int x, int y, int *tileset)
{
	int x_end, y_end;

	/* first find where x ends */
	for(x_end = x; x_end < editor.map->w; x_end++) {
		if(tileset[x_end + y * editor.map->w] >= 0) {
			break;
		}	
	}

	/* now, find y where the size does not fit x_end */
	for(y_end = y + 1; y_end < editor.map->h; y_end++) {
		for(int xx = x; xx < x_end; xx++) {
			if(tileset[xx + y_end * editor.map->w] >= 0) {
				goto produce_rect;
			}
		}
	}

produce_rect:
	if(x_end - x == 0 || y_end - y == 0)
		return;

	for(int yy = y; yy < y_end; yy++)
	for(int xx = x; xx < x_end; xx++) {
		tileset[xx + yy * editor.map->w] = 0;
	}
	
	fill_new_rect(x, y, (x_end - x), (y_end - y));
}

void
fill_new_rect(int x, int y, int w, int h)
{
	CollisionData *coll = malloc(sizeof(CollisionData));
	coll->half_size[0] = (float)w / 2;
	coll->half_size[1] = (float)h / 2;
	coll->position[0] = x + coll->half_size[0];
	coll->position[1] = y + coll->half_size[1];

	coll->next = editor.map->collision;
	editor.map->collision = coll;
}
