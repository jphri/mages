#include <stdbool.h>
#include <SDL.h>
#include <assert.h>

#include "graphics.h"
#include "util.h"
#include "global.h"
#include "vecmath.h"
#include "ui.h"

typedef struct {
	UIObjectType type;

	vec2 position;
	vec2 size;
	vec2 global_position;
	vec2 global_size;

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

#define UI_NODE(ID) ((UIObjectNode*)objalloc_data(&objects, ID))

static ObjectAllocator objects;

static inline void add_child(UIObject parent, UIObject child)
{
	UI_NODE(child)->sibling_next = UI_NODE(parent)->child_list;
	UI_NODE(child)->sibling_prev = 0;
	UI_NODE(parent)->child_list = child;

	UI_NODE(parent)->child_count ++;
}

static inline void remove_child(UIObject parent, UIObject child) 
{
	UIObject next, prev;

	next = UI_NODE(child)->sibling_next;
	prev = UI_NODE(child)->sibling_prev;
	if(prev) UI_NODE(prev)->sibling_next = next;
	if(next) UI_NODE(next)->sibling_prev = prev;

	if(UI_NODE(parent)->child_list == child)
		UI_NODE(parent)->child_list = next;

	UI_NODE(parent)->child_count --;
}

static void _sys_int_cleanup(ObjectAllocator *alloc, ObjectID id)
{
	(void)alloc;
	remove_child(UI_NODE(id)->parent, id);
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
	objalloc_init_allocator(&objects, sizeof(UIObjectNode), cache_aligned_allocator());
	objects.clean_cbk = _sys_int_cleanup;
}

void
ui_reset(void)
{
	objalloc_reset(&objects);
	hot = 0;
	active = 0;
}

void
ui_terminate(void)
{
	objalloc_end(&objects);
}

bool
ui_is_active(void)
{
	return hot || active;
}

UIObject
ui_new_object(UIObject parent, UIObjectType object_type) 
{
	UIObject object = objalloc_alloc(&objects);
	UI_NODE(object)->type = object_type;
	UI_NODE(object)->parent = parent;
	UI_NODE(object)->child_list = 0;
	UI_NODE(object)->child_count = 0;

	if(parent)
		add_child(parent, object); 
	else {
		UI_NODE(object)->sibling_next = 0;
		UI_NODE(object)->sibling_prev = 0;
	}

	return object;
}

void
ui_del_object(UIObject object) 
{
	UIObject child = UI_NODE(object)->child_list;
	while(child) {
		UIObject next = UI_NODE(child)->sibling_next;
		ui_del_object(child);
		child = next;
	}
	objalloc_free(&objects, child);
}

void
ui_draw(void) 
{
	gfx_draw_begin(NULL);
	for(UIObject child = objalloc_begin(&objects); child; child = objalloc_next(&objects, child))
		process_draw(child);
	gfx_draw_end();
}

void
ui_button_set_userptr(UIObject object, void *user_ptr)
{
	UI_NODE(object)->data.button.user_ptr = user_ptr;
}

void
ui_button_set_label(UIObject object, const char *label)
{
	UI_NODE(object)->data.button.label      = label;
	UI_NODE(object)->data.button.label_size = strlen(label);
}

void
ui_button_set_label_border(UIObject object, vec2 border)
{
	vec2_dup(UI_NODE(object)->data.button.label_border, border);
}

void 
ui_button_set_callback(UIObject object, void(*callback)(void *user_ptr))
{
	UI_NODE(object)->data.button.callback = callback;
}

void 
process_draw(UIObject object)
{
	#define OBJECT UI_NODE(object)

	if(ui_vtables[OBJECT->type].draw)
		ui_vtables[OBJECT->type].draw(object);

	for(UIObject child = OBJECT->child_list; child; child = UI_NODE(child)->sibling_next)
		process_draw(child);

	#undef OBJECT
}

void 
process_order(UIObject object)
{
	#define OBJECT UI_NODE(object)

	if(ui_vtables[OBJECT->type].order)
		ui_vtables[OBJECT->type].order(object);

	for(UIObject child = OBJECT->child_list; child; child = UI_NODE(child)->sibling_next)
		process_order(child);

	#undef OBJECT
}

void
process_mouse_motion(UIObject object, float x, float y) 
{
	#define OBJECT UI_NODE(object)

	for(UIObject child = OBJECT->child_list; child; child = UI_NODE(child)->sibling_next)
		process_mouse_motion(child, x, y);

	if(ui_vtables[OBJECT->type].mouse_motion)
		ui_vtables[OBJECT->type].mouse_motion(object, x, y);

	#undef OBJECT
}

void
process_mouse_button(UIObject object, UIMouseButton button, bool state) 
{
	#define OBJECT UI_NODE(object)

	for(UIObject child = OBJECT->child_list; child; child = UI_NODE(child)->sibling_next)
		process_mouse_button(child, button, state);

	if(ui_vtables[OBJECT->type].mouse_button)
		ui_vtables[OBJECT->type].mouse_button(object, button, state);

	#undef OBJECT
}

void
dummy_draw(UIObject object)
{
	#define OBJECT UI_NODE(object)
	(void)object;
	#undef OBJECT
}

void 
ui_obj_set_position(UIObject object, vec2 position)
{
	vec2_dup(UI_NODE(object)->position, position);
	vec2_dup(UI_NODE(object)->global_position, position);
}

void 
ui_obj_set_size(UIObject object, vec2 size)
{
	vec2_dup(UI_NODE(object)->size, size);
	vec2_dup(UI_NODE(object)->global_size, size);
}

void
button_draw(UIObject object) 
{
	#define BUTTON UI_NODE(object)->data.button
	#define OBJECT UI_NODE(object)

	TextureStamp white_rect = get_sprite(SPRITE_UI, 0, 0);

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

	gfx_draw_texture_rect(
			&white_rect,
			OBJECT->global_position,
			OBJECT->global_size,
			0.0,
			(vec4){ 0.7 + color_offset, 0.7 + color_offset, 0.7 + color_offset, 1.0 }
	);
	gfx_draw_font2(FONT_ROBOTO, label_position, 0.25, (vec4){ 0.0, 0.0, 0.0, 1.0 }, "%s", BUTTON.label);
	
	#undef BUTTON
	#undef OBJECT
}

void
label_draw(UIObject object) 
{
	#define LABEL UI_NODE(object)->data.label
	#define OBJECT UI_NODE(object)

	vec2 label_position, label_character_size;

	label_character_size[1] = OBJECT->global_size[1] - LABEL.border[1] * 2.0;
	label_character_size[0] = (OBJECT->global_size[0] - LABEL.border[0]) / LABEL.label_size;
	label_position[0] = OBJECT->global_position[0] - OBJECT->global_size[0] + label_character_size[0] + LABEL.border[0];
	label_position[1] = OBJECT->global_position[1];

	gfx_draw_font2(FONT_ROBOTO, label_position, 0.25, LABEL.color, "%s", LABEL.label_ptr);
	
	#undef LABEL
	#undef OBJECT
}

void 
window_draw(UIObject object)
{
	#define OBJECT UI_NODE(object)
	#define WINDOW UI_NODE(object)->data.window

	vec2 size;
	vec2_sub(size, OBJECT->global_size, WINDOW.border);
	TextureStamp white_rect = get_sprite(SPRITE_UI, 0, 0);

	gfx_draw_texture_rect(
			&white_rect,
			OBJECT->global_position,
			OBJECT->global_size,
			0.0,
			WINDOW.border_color
	);

	gfx_draw_texture_rect(
			&white_rect,
			OBJECT->global_position,
			size,
			0.0,
			WINDOW.background
	);

	#undef OBJECT
	#undef WINDOW
}

void
ui_mouse_motion(float x, float y)
{
	for(UIObject child = objalloc_begin(&objects); child; child = objalloc_next(&objects, child))
		process_mouse_motion(child, x, y);
}

void
ui_mouse_button(UIMouseButton button, bool state)
{
	for(UIObject child = objalloc_begin(&objects); child; child = objalloc_next(&objects, child))
		process_mouse_button(child, button, state);
}

void
ui_cleanup(void)
{
	objalloc_clean(&objects);
}

void
button_mouse_motion(UIObject object, float x, float y) 
{
	#define BUTTON UI_NODE(object)->data.button
	#define OBJECT UI_NODE(object)

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
	#define BUTTON UI_NODE(object)->data.button
	#define OBJECT UI_NODE(object)

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
	for(UIObject child = objalloc_begin(&objects); child; child = objalloc_next(&objects, child))
		process_order(child);
}

void
layout_order(UIObject object) 
{
	float dx = 0, dy = 0;
	float w = 0, h = 0;
	vec2 position;

	#define LAYOUT UI_NODE(object)->data.layout
	#define OBJECT UI_NODE(object)

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

	for(UIObject obj = OBJECT->child_list; obj; obj = UI_NODE(obj)->sibling_next) {
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
	#define WINDOW UI_NODE(object)->data.window
	#define OBJECT UI_NODE(object)

	for(UIObject obj = OBJECT->child_list; obj; obj = UI_NODE(obj)->sibling_next) {
		vec2 pos;
		vec2_sub(pos, OBJECT->global_position, OBJECT->global_size);

		vec2_add(UI_NODE(obj)->global_position, 
				UI_NODE(obj)->position,
				pos);
	}

	#undef WINDOW
	#undef OBJECT
}

void
ui_layout_set_order(UIObject object, UILayoutOrder order) 
{
	UI_NODE(object)->data.layout.order = order;
}

void 
ui_label_set_text(UIObject object, const char *text_ptr)
{
	UI_NODE(object)->data.label.label_ptr = text_ptr;
	UI_NODE(object)->data.label.label_size = strlen(text_ptr);
}

void ui_label_set_color(UIObject object, vec4 color)
{
	vec4_dup(UI_NODE(object)->data.label.color, color);
}

void 
ui_label_set_border(UIObject object, vec2 border)
{
	vec2_dup(UI_NODE(object)->data.label.border, border);
}

void 
ui_window_set_bg(UIObject object, vec4 color)
{
	vec4_dup(UI_NODE(object)->data.window.background, color);
}

void 
ui_window_set_border(UIObject object, vec4 color)
{
	vec4_dup(UI_NODE(object)->data.window.border_color, color);
}

void 
ui_window_set_border_size(UIObject object, vec2 size)
{
	vec2_dup(UI_NODE(object)->data.window.border, size);
}

