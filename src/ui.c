#include <stdbool.h>
#include <SDL2/SDL.h>
#include <assert.h>

#include "graphics.h"
#include "util.h"
#include "global.h"
#include "vecmath.h"
#include "ui.h"

typedef struct {
	UIObjectType type;
	UIObject next, prev;

	vec2 position;
	vec2 size;
	vec2 global_position;
	vec2 global_size;
	bool dead;

	UIObject parent;
	UIObject sibling_next, sibling_prev;
	UIObject child_list;

	size_t child_count;
	union {
		struct {
			const char *label;
			void *user_ptr;
			void (*callback)(void *userptr);
			vec2 label_border;
			size_t label_size;
		} button;

		struct {
			UILayoutOrder order;
		} layout;

		struct {
			const char *label_ptr;
			size_t label_size;
			vec4 color;
			vec2 border;
		} label;

		struct {
			vec2 border;
			vec4 border_color;
			vec4 background;
		} window;
	} data;
} UIObjectNode;

typedef struct UIElementVTable {
	void (*draw)(UIObject object);
	void (*mouse_motion)(UIObject object, float x, float y);
	void (*mouse_button)(UIObject object, UIMouseButton button, bool state);
	void (*order)(UIObject object);
} UIElementVTable;

static void process_draw(UIObject object);
static void process_order(UIObject object);
static void process_mouse_motion(UIObject object, float x, float y);
static void process_mouse_button(UIObject object, UIMouseButton button, bool state);

static void button_draw(UIObject object);
static void dummy_draw(UIObject object);
static void label_draw(UIObject object);
static void window_draw(UIObject object);

static void button_mouse_motion(UIObject object, float x, float y);
static void button_mouse_button(UIObject object, UIMouseButton button, bool state);
static void layout_order(UIObject object);
static void window_order(UIObject object);

#define SYS_ID_TYPE UIObject
#define SYS_NODE_TYPE UIObjectNode
#include "system.h"

static inline void add_child(UIObject parent, UIObject child)
{
	_sys_node(child)->sibling_next = _sys_node(parent)->child_list;
	_sys_node(child)->sibling_prev = 0;
	_sys_node(parent)->child_list = child;

	_sys_node(parent)->child_count ++;
}

static inline void remove_child(UIObject parent, UIObject child) 
{
	UIObject next, prev;

	next = _sys_node(child)->sibling_next;
	prev = _sys_node(child)->sibling_prev;
	if(prev) _sys_node(prev)->sibling_next = next;
	if(next) _sys_node(next)->sibling_prev = prev;

	if(_sys_node(parent)->child_list == child)
		_sys_node(parent)->child_list = next;

	_sys_node(parent)->child_count --;
}

static void _sys_int_cleanup(UIObject id)
{
	remove_child(_sys_node(id)->parent, id);
}

static UIElementVTable ui_vtables[] = {
	[UI_DUMMY] = {
		.draw = dummy_draw,
	},
	[UI_BUTTON] = {
		.draw = button_draw,
		.mouse_motion = button_mouse_motion,
		.mouse_button = button_mouse_button
	},
	[UI_LAYOUT] = {
		.order = layout_order
	},
	[UI_LABEL] = {
		.draw = label_draw,
	},
	[UI_WINDOW] = {
		.draw = window_draw,
		.order = window_order
	}
};
static UIObject hot, active;

void 
ui_init(void)
{
	_sys_init();
}

void
ui_reset(void)
{
	_sys_reset();
	hot = 0;
	active = 0;
}

void
ui_terminate(void)
{
	_sys_deinit();
}

bool
ui_is_active(void)
{
	return hot || active;
}

UIObject
ui_new_object(UIObject parent, UIObjectType object_type) 
{
	UIObject object = _sys_new();
	_sys_node(object)->type = object_type;
	_sys_node(object)->parent = parent;
	_sys_node(object)->child_list = 0;
	_sys_node(object)->child_count = 0;

	if(parent)
		add_child(parent, object); 
	else {
		_sys_node(object)->sibling_next = 0;
		_sys_node(object)->sibling_prev = 0;
	}

	return object;
}

void
ui_del_object(UIObject object) 
{
	UIObject child = _sys_node(object)->child_list;
	while(child) {
		UIObject next = _sys_node(child)->sibling_next;
		ui_del_object(child);
		child = next;
	}
	_sys_del(object);
}

void
ui_draw(void) 
{
	gfx_draw_begin(NULL);
	for(UIObject child = _sys_list; child; child = _sys_node(child)->next)
		process_draw(child);
	gfx_draw_end();
}

void
ui_button_set_userptr(UIObject object, void *user_ptr)
{
	_sys_node(object)->data.button.user_ptr = user_ptr;
}

void
ui_button_set_label(UIObject object, const char *label)
{
	_sys_node(object)->data.button.label      = label;
	_sys_node(object)->data.button.label_size = strlen(label);
}

void
ui_button_set_label_border(UIObject object, vec2 border)
{
	vec2_dup(_sys_node(object)->data.button.label_border, border);
}

void 
ui_button_set_callback(UIObject object, void(*callback)(void *user_ptr))
{
	_sys_node(object)->data.button.callback = callback;
}

void 
process_draw(UIObject object)
{
	#define OBJECT _sys_node(object)

	if(ui_vtables[OBJECT->type].draw)
		ui_vtables[OBJECT->type].draw(object);

	for(UIObject child = OBJECT->child_list; child; child = _sys_node(child)->sibling_next)
		process_draw(child);

	#undef OBJECT
}

void 
process_order(UIObject object)
{
	#define OBJECT _sys_node(object)

	if(ui_vtables[OBJECT->type].order)
		ui_vtables[OBJECT->type].order(object);

	for(UIObject child = OBJECT->child_list; child; child = _sys_node(child)->sibling_next)
		process_order(child);

	#undef OBJECT
}

void
process_mouse_motion(UIObject object, float x, float y) 
{
	#define OBJECT _sys_node(object)

	for(UIObject child = OBJECT->child_list; child; child = _sys_node(child)->sibling_next)
		process_mouse_motion(child, x, y);

	if(ui_vtables[OBJECT->type].mouse_motion)
		ui_vtables[OBJECT->type].mouse_motion(object, x, y);

	#undef OBJECT
}

void
process_mouse_button(UIObject object, UIMouseButton button, bool state) 
{
	#define OBJECT _sys_node(object)

	for(UIObject child = OBJECT->child_list; child; child = _sys_node(child)->sibling_next)
		process_mouse_button(child, button, state);

	if(ui_vtables[OBJECT->type].mouse_button)
		ui_vtables[OBJECT->type].mouse_button(object, button, state);

	#undef OBJECT
}

void
dummy_draw(UIObject object)
{
	#define OBJECT _sys_node(object)
	(void)object;
	#undef OBJECT
}

void 
ui_obj_set_position(UIObject object, vec2 position)
{
	vec2_dup(_sys_node(object)->position, position);
	vec2_dup(_sys_node(object)->global_position, position);
}

void 
ui_obj_set_size(UIObject object, vec2 size)
{
	vec2_dup(_sys_node(object)->size, size);
	vec2_dup(_sys_node(object)->global_size, size);
}

void
button_draw(UIObject object) 
{
	#define BUTTON _sys_node(object)->data.button
	#define OBJECT _sys_node(object)

	float color_offset = 0.0;
	vec2 label_position, label_character_size;

	if(hot == object)
		color_offset = 0.1;
	if(active == object)
		color_offset = -0.3;
		
	label_character_size[1] = OBJECT->global_size[1] - BUTTON.label_border[1] * 2.0;
	label_character_size[0] = (OBJECT->global_size[0] - BUTTON.label_border[0]) / BUTTON.label_size;
	label_position[0] = OBJECT->global_position[0] - OBJECT->global_size[0] + label_character_size[0] + BUTTON.label_border[0];
	label_position[1] = OBJECT->global_position[1];

	gfx_draw_sprite(&(Sprite) {
		.type = SPRITE_UI,
		.position = { OBJECT->global_position[0], OBJECT->global_position[1] },
		.color = { 0.7 + color_offset, 0.7 + color_offset, 0.7 + color_offset, 1.0 },
		.rotation = 0.0,
		.half_size = { OBJECT->global_size[0], OBJECT->global_size[1] },
		.sprite_id = { 0.0, 0.0 },
		.clip_region = { OBJECT->global_position[0], OBJECT->global_position[1], OBJECT->global_size[0], OBJECT->global_size[1] }
	});
	gfx_draw_font(TEXTURE_FONT_CELLPHONE, label_position, label_character_size, (vec4){ 0.0, 0.0, 0.0, 1.0 }, (vec4){ OBJECT->global_position[0], OBJECT->global_position[1], OBJECT->global_size[0], OBJECT->global_size[1] }, "%s", BUTTON.label);
	
	#undef BUTTON
	#undef OBJECT
}

void
label_draw(UIObject object) 
{
	#define LABEL _sys_node(object)->data.label
	#define OBJECT _sys_node(object)

	vec2 label_position, label_character_size;

	label_character_size[1] = OBJECT->global_size[1] - LABEL.border[1] * 2.0;
	label_character_size[0] = (OBJECT->global_size[0] - LABEL.border[0]) / LABEL.label_size;
	label_position[0] = OBJECT->global_position[0] - OBJECT->global_size[0] + label_character_size[0] + LABEL.border[0];
	label_position[1] = OBJECT->global_position[1];

	gfx_draw_font(TEXTURE_FONT_CELLPHONE, label_position, label_character_size, LABEL.color, (vec4){ OBJECT->global_position[0], OBJECT->global_position[1], OBJECT->global_size[0], OBJECT->global_size[1] }, "%s", LABEL.label_ptr);
	
	#undef LABEL
	#undef OBJECT
}

void 
window_draw(UIObject object)
{
	#define OBJECT _sys_node(object)
	#define WINDOW _sys_node(object)->data.window

	{
		vec2 size;
		vec2_sub(size, OBJECT->global_size, WINDOW.border);
		gfx_draw_sprite(&(Sprite) {
			.type = SPRITE_UI,
			.color = { WINDOW.border_color[0], WINDOW.border_color[1], WINDOW.border_color[2], WINDOW.border_color[3] },
			.position = { OBJECT->global_position[0], OBJECT->global_position[1] },
			.half_size = { OBJECT->global_size[0], OBJECT->global_size[1] },
			.clip_region = { OBJECT->global_position[0], OBJECT->global_position[1], OBJECT->global_size[0], OBJECT->global_size[1] }
		});

		gfx_draw_sprite(&(Sprite) {
			.type = SPRITE_UI,
			.color = { WINDOW.background[0], WINDOW.background[1], WINDOW.background[2], WINDOW.background[3] },
			.position = { OBJECT->global_position[0], OBJECT->global_position[1] },
			.half_size = { size[0], size[1] },
			.clip_region = { OBJECT->global_position[0], OBJECT->global_position[1], OBJECT->global_size[0], OBJECT->global_size[1] }
		});
	}

	#undef OBJECT
	#undef WINDOW
}

void
ui_mouse_motion(float x, float y)
{
	for(UIObject child = _sys_list; child; child = _sys_node(child)->next)
		process_mouse_motion(child, x, y);
}

void
ui_mouse_button(UIMouseButton button, bool state)
{
	for(UIObject child = _sys_list; child; child = _sys_node(child)->next)
		process_mouse_button(child, button, state);
}

void
ui_cleanup(void)
{
	_sys_cleanup();
}

void
button_mouse_motion(UIObject object, float x, float y) 
{
	#define BUTTON _sys_node(object)->data.button
	#define OBJECT _sys_node(object)

	if(active)
		return;

	vec2 min, max;
	vec2_add(max, OBJECT->global_position, OBJECT->global_size) ;
	vec2_sub(min, OBJECT->global_position, OBJECT->global_size) ;

	if(x > min[0] && x < max[0] && y > min[1] && y < max[1]) {
		hot = object;
	} else if(hot == object) {
		hot = 0;
	}

	#undef BUTTON
}

void
button_mouse_button(UIObject object, UIMouseButton button, bool state) 
{
	#define BUTTON _sys_node(object)->data.button
	#define OBJECT _sys_node(object)

	if(button != UI_MOUSE_LEFT)
		return;

	if(!state && active == object) {
		if(BUTTON.callback)
			BUTTON.callback(BUTTON.user_ptr);
	}

	if(state) {
		if(!active && hot == object)
			active = object;
	} else {
		if(active == object)
			active = 0;
	}

	#undef BUTTON
	#undef OBJECT
}

void
ui_order(void)
{
	for(UIObject child = _sys_list; child; child = _sys_node(child)->next)
		process_order(child);
}

void
layout_order(UIObject object) 
{
	float dx = 0, dy = 0;
	float w = 0, h = 0;
	vec2 position;

	#define LAYOUT _sys_node(object)->data.layout
	#define OBJECT _sys_node(object)

	switch(LAYOUT.order) {
	case UI_LAYOUT_HORIZONTAL:
		dx = 2 * OBJECT->global_size[0] / OBJECT->child_count;
		dy = 0;
		w = OBJECT->global_size[0] / OBJECT->child_count;
		h = OBJECT->global_size[1];
		break;
	case UI_LAYOUT_VERTICAL:
		dx = 0.0;
		dy = 2 * OBJECT->global_size[1] / OBJECT->child_count;

		w = OBJECT->global_size[0];
		h = OBJECT->global_size[1] / OBJECT->child_count;
		break;
	default:
		assert(0 && "SOMETHING DEFINETLY DID GO WRONG");
	}
	
	vec2_add_scaled(position, OBJECT->global_position, (vec2){ dx, dy }, ((float)OBJECT->child_count - 1)/ 2);

	for(UIObject obj = OBJECT->child_list; obj; obj = _sys_node(obj)->sibling_next) {
		ui_obj_set_size(obj, (vec2){ w, h });
		ui_obj_set_position(obj, position);
		vec2_sub(position, position, (vec2){ dx, dy });
	}

	#undef LAYOUT
	#undef OBJECT
}

void
window_order(UIObject object) 
{
	#define WINDOW _sys_node(object)->data.window
	#define OBJECT _sys_node(object)

	for(UIObject obj = OBJECT->child_list; obj; obj = _sys_node(obj)->sibling_next) {
		vec2 pos;
		vec2_sub(pos, OBJECT->global_position, OBJECT->global_size);

		vec2_add(_sys_node(obj)->global_position, 
				_sys_node(obj)->position,
				pos);
	}

	#undef WINDOW
	#undef OBJECT
}

void
ui_layout_set_order(UIObject object, UILayoutOrder order) 
{
	_sys_node(object)->data.layout.order = order;
}

void 
ui_label_set_text(UIObject object, const char *text_ptr)
{
	_sys_node(object)->data.label.label_ptr = text_ptr;
	_sys_node(object)->data.label.label_size = strlen(text_ptr);
}

void ui_label_set_color(UIObject object, vec4 color)
{
	vec4_dup(_sys_node(object)->data.label.color, color);
}

void 
ui_label_set_border(UIObject object, vec2 border)
{
	vec2_dup(_sys_node(object)->data.label.border, border);
}

void 
ui_window_set_bg(UIObject object, vec4 color)
{
	vec4_dup(_sys_node(object)->data.window.background, color);
}

void 
ui_window_set_border(UIObject object, vec4 color)
{
	vec4_dup(_sys_node(object)->data.window.border_color, color);
}

void 
ui_window_set_border_size(UIObject object, vec2 size)
{
	vec2_dup(_sys_node(object)->data.window.border, size);
}

