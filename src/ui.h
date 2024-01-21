#ifndef UI_H
#define UI_H
#include "vecmath.h"

#include <stdbool.h>

#define UI_WIDGET_LIST \
	UI_WIDGET(UI_WINDOW) \
	UI_WIDGET(UI_LAYOUT) \
	UI_WIDGET(UI_LABEL)  \
	UI_WIDGET(UI_BUTTON) \
	UI_WIDGET(UI_SLIDER) \
	UI_WIDGET(UI_FLOAT) 

typedef enum {
	UI_NULL,
	#define UI_WIDGET(NAME) NAME,
	UI_WIDGET_LIST
	#undef UI_WIDGET
	LAST_UI_WIDGET
} UIObjectType;

typedef enum {
	UI_MOUSE_LEFT,
	UI_MOUSE_MIDDLE, 
	UI_MOUSE_RIGHT
} UIMouseButton;

typedef enum {
	UI_LAYOUT_HORIZONTAL,
	UI_LAYOUT_VERTICAL
} UILayoutOrder;

typedef struct {
	enum {
		UI_MOUSE_MOTION,
		UI_MOUSE_BUTTON,
		UI_DRAW
	} event_type;

	union {
		struct {
			int button;
			int state;
			vec2 position;
		} mouse;
	} data;
} UIEvent;

typedef unsigned int UIObject;
typedef void UIEventProcessor(UIObject object, UIEvent *event, Rectangle *content);

#define DEFINE_WIDGET(NAME) \
typedef struct NAME##_struct NAME##_struct; \
struct NAME##_struct

DEFINE_WIDGET(UI_WINDOW) {
	const char *title;

	vec2 border;
	vec4 border_color;
	vec4 background;
	Rectangle window_rect;
};

DEFINE_WIDGET(UI_LABEL) {
	const char *label_ptr;
	size_t label_size;
	vec4 color;
	vec2 border;
};

DEFINE_WIDGET(UI_FLOAT) {
	Rectangle rect;
};

DEFINE_WIDGET(UI_LAYOUT) {
	UILayoutOrder order;
};

DEFINE_WIDGET(UI_BUTTON) {
	const char *label;
	void *user_ptr;
	void (*callback)(UIObject button_obj, void *userptr);
};

DEFINE_WIDGET(UI_SLIDER) {
	int max_value;
	int value;
};

void ui_init(void);
void ui_reset(void);
void ui_terminate(void);
bool ui_is_active(void);

UIObject ui_new_object(UIObject parent, UIObjectType object_type);
void     ui_del_object(UIObject object);

void ui_obj_set_position(UIObject object, vec2 position);
void ui_obj_set_size(UIObject object, vec2 size);
void ui_draw(void);

void ui_set_hot(UIObject object);
void ui_set_active(UIObject object);
UIObject ui_get_hot(void);
UIObject ui_get_active(void);

void ui_call_event(UIObject object, UIEvent *event, Rectangle *rect);
void *ui_data(UIObject obj);

UIObject ui_child(UIObject obj);
UIObject ui_child_next(UIObject child);
int      ui_count_child(UIObject obj);

void ui_button_set_label(UIObject object, const char *label);
void ui_button_set_label_border(UIObject object, vec2 border);
void ui_button_set_callback(UIObject object, void(*callback)(void *user_ptr));
void ui_button_set_userptr(UIObject object, void *ptr);
void ui_layout_set_order(UIObject object, UILayoutOrder layout_order);

void ui_label_set_text(UIObject object, const char *text_ptr);
void ui_label_set_color(UIObject object, vec4 color);
void ui_label_set_border(UIObject object, vec2 border);

void ui_update(void);
void ui_mouse_motion(float x, float y);
void ui_mouse_button(UIMouseButton button, bool state);
void ui_cleanup(void);

UIObject ui_root(void);
UIObject ui_get_parent(UIObject obj);
void     ui_default_mouse_handle(UIObject obj, UIEvent *event, Rectangle *rect);

#define UI_WIDGET(NAME) \
	UIEventProcessor NAME##_event;

UI_WIDGET_LIST

#undef UI_WIDGET

#endif
