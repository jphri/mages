#include <stdbool.h>
#include <SDL.h>
#include <assert.h>

#include "graphics.h"
#include "util.h"
#include "global.h"
#include "vecmath.h"
#include "ui.h"

typedef struct UIObjectNode UIObjectNode;
struct UIObjectNode {
	UIObjectType type;

	UIObject *parent, *next_parent;
	UIObjectNode *child_list;
	UIObjectNode *sibling_next, *sibling_prev;
	UIObjectNode *last_child;
	size_t child_count;

	bool deserted;
	enum {
		PREPEND,
		APPEND
	} new_child_mode;
	
	UIObject data;
};

typedef struct {
	UIObjectNode * child;
	UIObjectNode * to_parent;
} Reparent;

static ObjectPool objects;
static ArrayBuffer should_reparent;

static UIEventProcessor *procs[LAST_UI_WIDGET] =
{
	#define UI_WIDGET(NAME) \
		[NAME] = NAME##_event,

	UI_WIDGET_LIST

	#undef UI_WIDGET
};


static inline void add_child(UIObjectNode * parent, UIObjectNode * child)
{
	child->sibling_next = (parent)->child_list;
	child->sibling_prev = NULL;

	if(parent->last_child == NULL)
		parent->last_child = child;
	
	if(parent->child_list)
		(parent->child_list)->sibling_prev = child;

	parent->child_list = child;
	parent->child_count ++;
}

static inline void add_child_last(UIObjectNode * parent, UIObjectNode * child)
{
	UIObjectNode * last_child = (parent)->last_child;
	UIObjectNode * first_child = (parent)->child_list;

	(child)->sibling_prev = last_child;
	(child)->sibling_next = 0;

	if(!first_child)
		(parent)->child_list = child;
	
	if(last_child)
		(last_child)->sibling_next = child;

	(parent)->last_child = child;
	(parent)->child_count++;
}

static inline void remove_child(UIObjectNode * parent, UIObjectNode * child) 
{
	UIObjectNode *next, *prev;

	next = (child)->sibling_next;
	prev = (child)->sibling_prev;
	if(prev) (prev)->sibling_next = next;
	if(next) (next)->sibling_prev = prev;

	if((parent)->last_child == child)
		(parent)->last_child = prev;

	if((parent)->child_list == child)
		(parent)->child_list = next;

	(parent)->child_count --;
}

static void cleancbk(ObjectPool *obj, void *);

static UIObject *hot, *active, *text_active;
static UIObject *root;
static vec2 mouse_pos;

void 
ui_init(void)
{
	arrbuf_init(&should_reparent);
	objpool_init(&objects, sizeof(UIObjectNode), DEFAULT_ALIGNMENT);
	objects.clean_cbk = cleancbk;
	
	ui_reset();
}

void
ui_reset(void)
{
	for(UIObjectNode * obj = objpool_begin(&objects);
			obj;
			obj = objpool_next(obj))
	{
		objpool_free(obj);
	}
	ui_cleanup();

	arrbuf_clear(&should_reparent);
	root = ui_new_object(0, UI_ROOT);
	hot = NULL;
	active = NULL;
}

void
ui_terminate(void)
{
	ui_reset();
	ui_cleanup();

	arrbuf_free(&should_reparent);
	objpool_terminate(&objects);
}

bool
ui_is_active(void)
{
	return hot || active;
}

UIObject *
ui_new_object(UIObject *parent, UIObjectType object_type) 
{
	UIObjectNode *object = objpool_new(&objects);
	memset(object, 0, sizeof(UIObjectNode));
	object->type         = object_type;
	object->parent       = parent;

	if(parent)
		add_child(CONTAINER_OF(parent, UIObjectNode, data), object); 

	return& object->data;
}

void
ui_del_object(UIObject * object) 
{
	UIObjectNode * child = CONTAINER_OF(object, UIObjectNode, data)->child_list;
	while(child) {
		UIObjectNode *next = (child)->sibling_next;
		ui_del_object(&child->data);
		child = next;
	}
	ui_deparent(object);
	objpool_free(CONTAINER_OF(object, UIObjectNode, data));
}

void
ui_draw(void) 
{
	UIEvent event;
	Rectangle rect = gfx_window_rectangle();

	event.event_type = UI_DRAW;

	gfx_begin();
	ui_call_event(root, &event, &rect);
	gfx_flush();
	gfx_end();
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

	ui_call_event(root, &event, &rect);
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

	ui_set_hot(0);
	ui_call_event(root, &event, &rect);
}

void
ui_cleanup(void)
{
	Span reparent = arrbuf_span(&should_reparent);
	SPAN_FOR(reparent, dp, UIObjectNode *) {
		if((*dp)->parent)
			remove_child(CONTAINER_OF((*dp)->parent, UIObjectNode, data), *dp);

		if((*dp)->next_parent) {
			switch((*dp)->new_child_mode) {
			case PREPEND:
				add_child(CONTAINER_OF((*dp)->next_parent, UIObjectNode, data), *dp);
				break;
			case APPEND:
				add_child_last(CONTAINER_OF((*dp)->next_parent, UIObjectNode, data), *dp);
				break;
			}
		}
		(*dp)->deserted = false;
		(*dp)->parent = (*dp)->next_parent;
	}
	arrbuf_clear(&should_reparent);
	objpool_clean(&objects);
}

void
ui_call_event(UIObject * object, UIEvent *event, Rectangle *rect)
{
	procs[CONTAINER_OF(object, UIObjectNode, data)->type](object, event, rect);
}

UIObject *
ui_child(UIObject *parent)
{
	UIObjectNode * list = CONTAINER_OF(parent, UIObjectNode, data)->child_list;
	while(list && objpool_is_dead(list)) {
		list = (list)->sibling_next;
	}
	if(!list)
		return NULL;
	return &list->data;
}

UIObject *
ui_child_next(UIObject * child)
{
	UIObjectNode *list = CONTAINER_OF(child, UIObjectNode, data)->sibling_next;
	while(list && objpool_is_dead(list) && (list)->deserted) {
		list = (list)->sibling_next;
	}
	if(!list)
		return NULL;
	return &list->data;
}

UIObject *
ui_last_child(UIObject * parent)
{
	if(!CONTAINER_OF(parent, UIObjectNode, data)->last_child)
		return NULL;
	return &CONTAINER_OF(parent, UIObjectNode, data)->last_child->data;
}

UIObject *
ui_child_prev(UIObject * obj)
{
	if(!CONTAINER_OF(obj, UIObjectNode, data)->sibling_prev)
		return NULL;
	return &CONTAINER_OF(obj, UIObjectNode, data)->sibling_prev->data;
}

UIObject *
ui_root(void)
{
	return root;
}

void
ui_map(UIObject *obj)
{
	ui_child_append(root, obj);
}

int
ui_count_child(UIObject * obj)
{
	return CONTAINER_OF(obj, UIObjectNode, data)->child_count;
}

void
ui_set_hot(UIObject * obj)
{
	hot = obj;
}

void
ui_set_active(UIObject * obj)
{
	active = obj;
}

UIObject *
ui_get_hot(void)
{
	return hot;
}

UIObject *
ui_get_active(void)
{
	return active;
}

UIObject *
ui_get_parent(UIObject * obj)
{
	return CONTAINER_OF(obj, UIObjectNode, data)->parent;
}

void
ui_set_text_active(UIObject * obj)
{
	text_active = obj;
	if(obj)
		enable_text_input();
	else
	 	disable_text_input();
}

UIObjectType
ui_get_type(UIObject * obj)
{
	return CONTAINER_OF(obj, UIObjectNode, data)->type;
}

void
ui_position_translate(UIPosition *pos, Rectangle *bound, vec2 out_pos)
{
	vec2 min, max;
	rect_boundaries(min, max, bound);

	switch(pos->origin) {
	case UI_ORIGIN_TOP_LEFT:     vec2_add(out_pos, pos->position, (vec2){ min[0], min[1] }); break;
	case UI_ORIGIN_TOP_RIGHT:    vec2_add(out_pos, pos->position, (vec2){ max[0], min[1] }); break;
	case UI_ORIGIN_BOTTOM_LEFT:  vec2_add(out_pos, pos->position, (vec2){ min[0], max[1] }); break;
	case UI_ORIGIN_BOTTOM_RIGHT: vec2_add(out_pos, pos->position, (vec2){ max[0], max[1] }); break;
	case UI_ORIGIN_CENTER:       vec2_add(out_pos, pos->position, bound->position); break;
	case UI_ORIGIN_ABSOLUTE:     vec2_dup(out_pos, pos->position); break;
	}
}

UIObject *
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
	ui_call_event(root, &event, &rect);
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
	ui_call_event(root, &event, &rect);
}

void
ui_child_prepend(UIObject *parent, UIObject *child)
{
	UIObjectNode *child_node = CONTAINER_OF(child, UIObjectNode, data);
	child_node->deserted = true;
	child_node->next_parent = parent;
	child_node->new_child_mode = PREPEND;
	arrbuf_insert(&should_reparent, sizeof(child_node), &child_node);
}

void
ui_child_append(UIObject * parent, UIObject * child)
{
	UIObjectNode *child_node = CONTAINER_OF(child, UIObjectNode, data);

	(child_node)->deserted = true;
	(child_node)->next_parent = parent;
	(child_node)->new_child_mode = APPEND;
	arrbuf_insert(&should_reparent, sizeof(child_node), &child_node);
}

void
ui_default_mouse_handle(UIObject * obj, UIEvent *ev, Rectangle *rect)
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
ui_deparent(UIObject * obj)
{
	ui_child_append(0, obj);
}

static void
cleancbk(ObjectPool *pool, void *obj)
{
	(void)pool;
	UIObjectNode *node = obj;
	Rectangle rect = gfx_window_rectangle();
	ui_call_event(&node->data, &(UIEvent) {
		.event_type = UI_DELETE
	}, &rect);
}
