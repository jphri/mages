#ifndef UI_H
#define UI_H

#include "vecmath.h"
#include "util.h"
#include "graphics.h"

#include <stdbool.h>

#define UI_WIDGET_LIST       \
	UI_WIDGET(UI_WINDOW)     \
	UI_WIDGET(UI_LAYOUT)     \
	UI_WIDGET(UI_LABEL)      \
	UI_WIDGET(UI_BUTTON)     \
	UI_WIDGET(UI_SLIDER)     \
	UI_WIDGET(UI_FLOAT)      \
	UI_WIDGET(UI_TEXT_INPUT) \
	UI_WIDGET(UI_IMAGE)      

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
	UI_KEY_LEFT,
	UI_KEY_RIGHT,
	UI_KEY_UP,
	UI_KEY_DOWN,
	UI_KEY_BACKSPACE,
	UI_KEY_ENTER
} UIKey;

typedef enum {
	UI_LAYOUT_HORIZONTAL,
	UI_LAYOUT_VERTICAL
} UILayoutOrder;

typedef enum {
	UI_LABEL_ALIGN_LEFT,
	UI_LABEL_ALIGN_CENTER,
	UI_LABEL_ALIGN_RIGHT
} UILabelAlign;

typedef struct {
	enum {
		UI_MOUSE_MOTION,
		UI_MOUSE_BUTTON,
		UI_DRAW,
		UI_TEXT_ENTRY,
		UI_KEYBOARD
	} event_type;

	union {
		struct {
			int button;
			int state;
			vec2 position;
		} mouse;
		struct {
			const char *text;
			size_t      text_size;
		} text;

		UIKey key;
	} data;
} UIEvent;

typedef unsigned int UIObject;
typedef void UIEventProcessor(UIObject object, UIEvent *event, Rectangle *content);

#define DEFINE_WIDGET(NAME) \
typedef struct NAME##_struct NAME##_struct; \
struct NAME##_struct

#define WIDGET(NAME, ID) ((NAME##_struct*)ui_data(ID))

DEFINE_WIDGET(UI_WINDOW) {
	const char *title;

	vec2 border;
	vec4 border_color;
	vec4 background;
	Rectangle window_rect;

	UIObject title_label;
	UIObject title_layout;
	UIObject child;

	vec2 drag_begin;
	vec2 drag_begin_pos;

	bool decorated;
};

DEFINE_WIDGET(UI_LABEL) {
	const char *label_ptr;
	size_t label_size;
	vec4 color;
	vec2 border;

	UILabelAlign align;
};

DEFINE_WIDGET(UI_FLOAT) {
	Rectangle rect;
};

DEFINE_WIDGET(UI_LAYOUT) {
	UILayoutOrder order;
	vec4 border;
	vec4 background;
};

DEFINE_WIDGET(UI_BUTTON) {
	void *user_ptr;
	void (*callback)(UIObject button_obj, void *userptr);
	UIObject label;
};

DEFINE_WIDGET(UI_SLIDER) {
	int max_value;
	int value, old_value;
	void *user_ptr;
	void (*cbk)(UIObject, void *);
};

DEFINE_WIDGET(UI_TEXT_INPUT) {
	ArrayBuffer text_buffer;
	int carot;
	float offset;
};

DEFINE_WIDGET(UI_IMAGE) {
	TextureStamp image_stamp;
	bool keep_aspect;
};

void ui_init(void);
void ui_reset(void);
void ui_terminate(void);
bool ui_is_active(void);

UIObject ui_window_new(void);
void     ui_window_set_size(UIObject window, vec2 size);
void     ui_window_set_position(UIObject window, vec2 position);
void     ui_window_set_background(UIObject window, vec4 background);
void     ui_window_set_title(UIObject window, const char *title);
void     ui_window_set_border(UIObject window, vec2 border);
void     ui_window_set_child(UIObject window, UIObject child);
void     ui_window_set_decorated(UIObject window, bool decorated);

UIObject ui_layout_new(void);
void     ui_layout_set_order(UIObject layout, UILayoutOrder order);
void     ui_layout_set_border(UIObject layout, float left, float right, float up, float down);
void     ui_layout_set_background(UIObject layout, vec4 color);
void     ui_layout_append(UIObject layout, UIObject child);

UIObject ui_button_new(void);
void     ui_button_set_label(UIObject object, UIObject label);
void     ui_button_set_callback(UIObject object, void *ptr, void(*callback)(UIObject button_obj, void *user_ptr));

UIObject ui_label_new(void);
void     ui_label_set_text(UIObject object, const char *text_ptr);
void     ui_label_set_color(UIObject object, vec4 color);
void     ui_label_set_border(UIObject object, vec2 border);
void     ui_label_set_alignment(UIObject object, UILabelAlign align);

UIObject ui_slider_new(void);
void     ui_slider_set_max_value(UIObject slider, int max_value);
void     ui_slider_set_value(UIObject slider, int value);
int      ui_slider_get_value(UIObject slider);
void     ui_slider_set_callback(UIObject slider, void *userptr, void (*cbk)(UIObject, void *));

UIObject ui_text_input_new(void);
StrView  ui_text_input_get_str(UIObject obj);

UIObject ui_image_new(void);
void     ui_image_set_stamp(UIObject image, TextureStamp *stamp);
void     ui_image_set_keep_aspect(UIObject image, bool keep);

void     ui_map(UIObject obj);

UIObject ui_new_object(UIObject parent, UIObjectType object_type);
void     ui_del_object(UIObject object);
void     ui_draw(void);

void ui_set_hot(UIObject object);
void ui_set_active(UIObject object);
void ui_set_text_active(UIObject object);
UIObject ui_get_hot(void);
UIObject ui_get_active(void);
UIObject ui_get_text_active(void);

void ui_call_event(UIObject object, UIEvent *event, Rectangle *rect);
void *ui_data(UIObject obj);

UIObject ui_child(UIObject parent);
UIObject ui_last_child(UIObject parent);
UIObject ui_child_next(UIObject child);
UIObject ui_child_prev(UIObject object);
int      ui_count_child(UIObject obj);
void     ui_child_append(UIObject parent, UIObject child);
void     ui_child_prepend(UIObject parent, UIObject child);

void ui_update(void);
void ui_mouse_motion(float x, float y);
void ui_mouse_button(UIMouseButton button, bool state);
void ui_text(const char *buffer, size_t text_size);
void ui_key(UIKey key);
void ui_cleanup(void);

UIObject ui_root(void);
UIObject ui_get_parent(UIObject obj);
void     ui_default_mouse_handle(UIObject obj, UIEvent *event, Rectangle *rect);

#define UI_WIDGET(NAME) \
	UIEventProcessor NAME##_event;

UI_WIDGET_LIST

#undef UI_WIDGET

#endif
