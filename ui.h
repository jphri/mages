typedef enum {
	UI_NULL,
	UI_DUMMY,
	UI_LAYOUT,
	UI_LABEL,
	UI_WINDOW,
	UI_BUTTON,
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

typedef float vec2[2];
typedef float vec4[4];

typedef unsigned int UIObject;

void ui_init();
void ui_terminate();
bool ui_is_active();

UIObject ui_new_object(UIObject parent, UIObjectType object_type);
void     ui_del_object(UIObject object);

void ui_obj_set_position(UIObject object, vec2 position);
void ui_obj_set_size(UIObject object, vec2 size);
void ui_draw();

void ui_button_set_label(UIObject object, const char *label);
void ui_button_set_label_border(UIObject object, vec2 border);
void ui_button_set_callback(UIObject object, void(*callback)(void *user_ptr));
void ui_button_set_userptr(UIObject object, void *ptr);

void ui_layout_set_order(UIObject object, UILayoutOrder layout_order);

void ui_label_set_text(UIObject object, const char *text_ptr);
void ui_label_set_color(UIObject object, vec4 color);
void ui_label_set_border(UIObject object, vec2 border);

void ui_window_set_bg(UIObject object, vec4 color);
void ui_window_set_border(UIObject object, vec4 color);
void ui_window_set_border_size(UIObject object, vec2 size);

void ui_order();
void ui_mouse_motion(float x, float y);
void ui_mouse_button(UIMouseButton button, bool state);
void ui_cleanup();
