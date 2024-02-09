#include "../ui.h"
#include "../graphics.h"

static void button_motion(UIObject obj, UIEvent *event, Rectangle *rect);
static void button_mouse(UIObject obj, UIEvent *event);
static void button_draw(UIObject obj, UIEvent *event, Rectangle *rect);

UIObject 
ui_button_new(void)
{
	UIObject obj = ui_new_object(0, UI_BUTTON);
	ui_button_set_label(obj, 0);
	ui_button_set_callback(obj, NULL, NULL);
	return obj;
}

void
ui_button_set_label(UIObject object, UIObject label)
{
	UI_BUTTON_struct *btn = ui_data(object);
	if(label)
		ui_child_append(object, label);
	btn->label = label;
}

void
ui_button_set_callback(UIObject object, void *ptr, void(*callback)(UIObject button_obj, void *user_ptr))
{
	UI_BUTTON_struct *btn = ui_data(object);
	btn->user_ptr = ptr;
	btn->callback = callback;
}

void 
UI_BUTTON_event(UIObject obj, UIEvent *ev, Rectangle *content)
{
	ui_default_mouse_handle(obj, ev, content);
	switch(ev->event_type) {
	case UI_DRAW:
		button_draw(obj, ev, content);
		break;

	case UI_MOUSE_MOTION:
		button_motion(obj, ev, content);
		break;

	case UI_MOUSE_BUTTON:
		button_mouse(obj, ev);
		break;
	
	default:
		do {} while(0);
	}
}

static void
button_motion(UIObject obj, UIEvent *event, Rectangle *rect)
{
	(void)obj;
	(void)event;
	(void)rect;
}

static void
button_mouse(UIObject obj, UIEvent *event)
{
	UI_BUTTON_struct *data = ui_data(obj);

	if(ui_get_active() == 0) {
		if(ui_get_hot() == obj) {
			if(event->data.mouse.button == UI_MOUSE_LEFT && event->data.mouse.state)
				ui_set_active(obj);
		}
	}

	if(ui_get_active() == obj) {
		if(event->data.mouse.button == UI_MOUSE_LEFT && !event->data.mouse.state) {
			if(data->callback)
				data->callback(obj, data->user_ptr);
			ui_set_active(0);
		}
	}
}

static void 
button_draw(UIObject obj, UIEvent *event, Rectangle *rect)
{
	UI_BUTTON_struct *button = ui_data(obj);

	if(ui_get_active() == obj) {
		gfx_draw_texture_rect(gfx_white_texture(), rect->position, rect->half_size, 0.0, (vec4){ 0.4, 0.4, 0.4, 1.0 });
	} else if(ui_get_hot() == obj) {
		gfx_draw_texture_rect(gfx_white_texture(), rect->position, rect->half_size, 0.0, (vec4){ 0.8, 0.8, 0.8, 1.0 });
	} else 
		gfx_draw_texture_rect(gfx_white_texture(), rect->position, rect->half_size, 0.0, (vec4){ 0.6, 0.6, 0.6, 1.0 });

	gfx_push_clip(rect->position, rect->half_size);
	if(button->label)
			ui_call_event(button->label, event, rect);
	gfx_pop_clip();
}
