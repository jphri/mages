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

	UIObject parent, next_parent;
	UIObject sibling_next, sibling_prev;
	UIObject child_list;
	UIObject last_child;
	size_t child_count;

	bool deserted;
	enum {
		PREPEND,
		APPEND
	} new_child_mode;

	union {
		#define UI_WIDGET(NAME) \
			NAME##_struct __##NAME;
		UI_WIDGET_LIST
		#undef UI_WIDGET
	} data;
} UIObjectNode;

typedef struct {
	UIObject child;
	UIObject to_parent;
} Reparent;

#define UI_NODE(ID) ((UIObjectNode*)objalloc_data(&objects, ID))

static ObjectAllocator objects;
static ArrayBuffer should_reparent;

static UIEventProcessor *procs[LAST_UI_WIDGET] =
{
	#define UI_WIDGET(NAME) \
		[NAME] = NAME##_event,

	UI_WIDGET_LIST

	#undef UI_WIDGET
};


static inline void add_child(UIObject parent, UIObject child)
{
	UI_NODE(child)->sibling_next = UI_NODE(parent)->child_list;
	UI_NODE(child)->sibling_prev = 0;

	if(UI_NODE(parent)->last_child == 0)
		UI_NODE(parent)->last_child = child;
	
	if(UI_NODE(parent)->child_list)
		UI_NODE(UI_NODE(parent)->child_list)->sibling_prev = child;

	UI_NODE(parent)->child_list = child;
	UI_NODE(parent)->child_count ++;
}

static inline void add_child_last(UIObject parent, UIObject child)
{
	UIObject last_child = UI_NODE(parent)->last_child;
	UIObject first_child = UI_NODE(parent)->child_list;

	UI_NODE(child)->sibling_prev = last_child;
	UI_NODE(child)->sibling_next = 0;

	if(!first_child)
		UI_NODE(parent)->child_list = child;
	
	if(last_child)
		UI_NODE(last_child)->sibling_next = child;

	UI_NODE(parent)->last_child = child;
	UI_NODE(parent)->child_count++;
}

static inline void remove_child(UIObject parent, UIObject child) 
{
	UIObject next, prev;

	next = UI_NODE(child)->sibling_next;
	prev = UI_NODE(child)->sibling_prev;
	if(prev) UI_NODE(prev)->sibling_next = next;
	if(next) UI_NODE(next)->sibling_prev = prev;

	if(UI_NODE(parent)->last_child == child)
		UI_NODE(parent)->last_child = prev;

	if(UI_NODE(parent)->child_list == child)
		UI_NODE(parent)->child_list = next;

	UI_NODE(parent)->child_count --;
}

static UIObject hot, active, text_active;
static UIObject root;
static vec2 mouse_pos;

void 
ui_init(void)
{
	arrbuf_init(&should_reparent);
	objalloc_init_allocator(&objects, sizeof(UIObjectNode), cache_aligned_allocator());
	
	ui_reset();
}

void
ui_reset(void)
{
	arrbuf_clear(&should_reparent);
	objalloc_reset(&objects);
	root = ui_new_object(0, UI_NULL);
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
	return hot != root || active;
}

UIObject
ui_new_object(UIObject parent, UIObjectType object_type) 
{
	UIObject object = objalloc_alloc(&objects);
	memset(objalloc_data(&objects, object), 0, sizeof(UIObjectNode));
	UI_NODE(object)->type         = object_type;
	UI_NODE(object)->parent       = parent;


	if(parent)
		add_child(parent, object); 

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
	ui_deparent(object);
	objalloc_free(&objects, object);
}

void
ui_draw(void) 
{
	UIEvent event;
	Rectangle rect = gfx_window_rectangle();

	event.event_type = UI_DRAW;

	gfx_draw_begin(NULL);
	/* the only retarded that do this in reverse is the root, lol */
	/* lets laugh at the UI_NULL */
	/* loooooooooooooool */
	for(UIObject child = UI_NODE(root)->last_child; child; child = UI_NODE(child)->sibling_prev) {
		ui_call_event(child, &event, &rect);
	}
	gfx_draw_end();
}

void
ui_mouse_button(UIMouseButton button, bool state)
{
	UIEvent event;
	Rectangle rect = gfx_window_rectangle();

	event.event_type = UI_MOUSE_BUTTON;
	event.data.mouse.state = state;
	event.data.mouse.button = button;
	vec2_dup(event.data.mouse.position, mouse_pos);

	for(UIObject child = ui_child(root); child; child = ui_child_next(child)) {
		ui_call_event(child, &event, &rect);
	}
}

void
ui_mouse_motion(float x, float y)
{
	UIEvent event;
	Rectangle rect = gfx_window_rectangle();

	event.event_type = UI_MOUSE_MOTION;
	event.data.mouse.position[0] = x;
	event.data.mouse.position[1] = y;
	vec2_dup(mouse_pos, event.data.mouse.position);

	ui_default_mouse_handle(root, &event, &rect);
	for(UIObject child = ui_child(root); child; child = ui_child_next(child)) {
		ui_call_event(child, &event, &rect);
	}
}

void
ui_cleanup(void)
{
	Span reparent = arrbuf_span(&should_reparent);
	SPAN_FOR(reparent, dp, UIObject) {
		if(ui_get_parent(*dp))
			remove_child(ui_get_parent(*dp), *dp);

		if(UI_NODE(*dp)->next_parent) {
			switch(UI_NODE(*dp)->new_child_mode) {
			case PREPEND:
				add_child(UI_NODE(*dp)->next_parent, *dp);
				break;
			case APPEND:
				add_child_last(UI_NODE(*dp)->next_parent, *dp);
				break;
			}
		}
		UI_NODE(*dp)->parent = UI_NODE(*dp)->next_parent;
	}
	objalloc_clean(&objects);
}

void
ui_call_event(UIObject object, UIEvent *event, Rectangle *rect)
{
	procs[UI_NODE(object)->type](object, event, rect);
}

void *
ui_data(UIObject object) 
{
	return &UI_NODE(object)->data;
}

UIObject
ui_child(UIObject parent)
{
	UIObject list = UI_NODE(parent)->child_list;
	while(list && objalloc_is_dead(&objects, list)) {
		list = UI_NODE(list)->sibling_next;
	}
	return list;
}

UIObject
ui_child_next(UIObject child)
{
	child = UI_NODE(child)->sibling_next;
	while(child && objalloc_is_dead(&objects, child) && UI_NODE(child)->deserted) {
		child = UI_NODE(child)->sibling_next;
	}
	return child;
}

UIObject
ui_root(void)
{
	return root;
}

void
ui_map(UIObject obj)
{
	ui_child_append(root, obj);
}

int
ui_count_child(UIObject obj)
{
	return UI_NODE(obj)->child_count;
}

void
ui_set_hot(UIObject obj)
{
	hot = obj;
}

void
ui_set_active(UIObject obj)
{
	active = obj;
}

UIObject
ui_get_hot(void)
{
	return hot;
}

UIObject
ui_get_active(void)
{
	return active;
}

UIObject
ui_get_parent(UIObject obj)
{
	return UI_NODE(obj)->parent;
}

void
ui_set_text_active(UIObject obj)
{
	text_active = obj;
	if(obj)
		enable_text_input();
	else
	 	disable_text_input();
}

UIObject
ui_get_text_active(void)
{
	return text_active;
}

void
ui_text(const char *text, size_t text_size)
{
	UIEvent event;
	Rectangle rect = gfx_window_rectangle();

	event.event_type = UI_TEXT_ENTRY;
	event.data.text.text = text;
	event.data.text.text_size = text_size;

	if(!text_active)
		return;

	/* even if we know what element is requesting the
	 * text data, we need to repass to its parents too
	 */
	for(UIObject child = ui_child(root); child; child = ui_child_next(child)) {
		ui_call_event(child, &event, &rect);
	}
}

void
ui_key(UIKey key)
{
	UIEvent event;
	Rectangle rect = gfx_window_rectangle();

	event.event_type = UI_KEYBOARD;
	event.data.key   = key;

	if(!text_active)
		return;

	/* even if we know what element is requesting the
	 * text data, we need to repass to its parents too
	 */
	for(UIObject child = ui_child(root); child; child = ui_child_next(child)) {
		ui_call_event(child, &event, &rect);
	}
}

void
ui_child_prepend(UIObject parent, UIObject child)
{
	UI_NODE(child)->deserted = true;
	UI_NODE(child)->next_parent = parent;
	UI_NODE(child)->new_child_mode = PREPEND;
	arrbuf_insert(&should_reparent, sizeof(child), &child);
}

void
ui_child_append(UIObject parent, UIObject child)
{
	UI_NODE(child)->deserted = true;
	UI_NODE(child)->next_parent = parent;
	UI_NODE(child)->new_child_mode = APPEND;
	arrbuf_insert(&should_reparent, sizeof(child), &child);
}

void
ui_default_mouse_handle(UIObject obj, UIEvent *ev, Rectangle *rect)
{
	if(ev->event_type != UI_MOUSE_MOTION && ev->event_type != UI_MOUSE_BUTTON)
		return;

	if(ui_get_active() != 0)
		return;

	if(ui_get_hot() == ui_get_parent(obj)) {
		if(rect_contains_point(rect, ev->data.mouse.position))
			ui_set_hot(obj);
	}
	if(ui_get_hot() == obj) {
		if(!rect_contains_point(rect, ev->data.mouse.position))
			ui_set_hot(ui_get_parent(obj));
	}
}

void
ui_deparent(UIObject obj)
{
	ui_child_append(0, obj);
}
