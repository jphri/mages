#include "../ui.h"

#define CHECKBOX(obj) WIDGET(UI_CHECKBOX, obj)

static void checkbox_draw(UIObject obj, Rectangle *rect);
static void checkbox_button(UIObject obj, UIEvent *event);

UIObject
ui_checkbox_new(void)
{
	UIObject obj = ui_new_object(0, UI_CHECKBOX);
	CHECKBOX(obj)->callback = NULL;
	CHECKBOX(obj)->toggled = 0;
	CHECKBOX(obj)->userptr = NULL;
	return obj;
}

void
ui_checkbox_set_callback(UIObject obj, void *userptr, void (*cbk)(UIObject, void *))
{
	CHECKBOX(obj)->callback = cbk;
	CHECKBOX(obj)->userptr = userptr;
}

bool
ui_checkbox_get_toggled(UIObject obj)
{
	return CHECKBOX(obj)->toggled;
}

void
ui_checkbox_set_toggled(UIObject obj, bool toggled)
{
	CHECKBOX(obj)->toggled = toggled;
}

void
UI_CHECKBOX_event(UIObject obj, UIEvent *event, Rectangle *rect)
{
	Rectangle actual_rect = *rect;
	float aspect = fminf(actual_rect.half_size[0], actual_rect.half_size[1]);
	actual_rect.half_size[0] = aspect;
	actual_rect.half_size[1] = aspect;

	ui_default_mouse_handle(obj, event, &actual_rect);
	switch(event->event_type) {
	case UI_DRAW: checkbox_draw(obj, &actual_rect); break;
	case UI_MOUSE_BUTTON: checkbox_button(obj, event); break;
	default: break;
	}
}

void
checkbox_draw(UIObject obj, Rectangle *rect)
{
	TextureStamp stamp = get_sprite(SPRITE_UI, 0 + CHECKBOX(obj)->toggled, 1);
	gfx_draw_texture_rect(&stamp, rect->position, rect->half_size, 0.0, (vec4){ 1.0, 1.0, 1.0, 1.0 });
}

void
checkbox_button(UIObject obj, UIEvent *event)
{
	if(ui_get_active() == 0) {
		if(ui_get_hot() == obj) {
			if(event->data.mouse.button == UI_MOUSE_LEFT && event->data.mouse.state)
				ui_set_active(obj);
		}
	}

	if(ui_get_active() == obj) {
		if(event->data.mouse.button == UI_MOUSE_LEFT && !event->data.mouse.state) {
			if(CHECKBOX(obj)->callback)
				CHECKBOX(obj)->callback(obj, CHECKBOX(obj)->userptr);
			ui_set_active(0);
		}
	}
}
