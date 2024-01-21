#include "../ui.h"
#include "../graphics.h"

static void
button_motion(UIObject obj, UIEvent *event, Rectangle *rect)
{
	if(ui_get_hot() == obj) {
		if(!rect_contains_point(rect, event->data.mouse.position))
			ui_set_hot(0);
	} else {
		if(ui_get_hot() == ui_get_parent(obj) && rect_contains_point(rect, event->data.mouse.position))
			ui_set_hot(obj);
	}
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
button_draw(UIObject obj, Rectangle *rect)
{
	UI_BUTTON_struct *button = ui_data(obj);
	vec2 content_size;
	vec2 content_pos;

	if(ui_get_active() == obj) {
		gfx_draw_texture_rect(gfx_white_texture(), rect->position, rect->half_size, 0.0, (vec4){ 0.4, 0.4, 0.4, 1.0 });
	} else if(ui_get_hot() == obj) {
		gfx_draw_texture_rect(gfx_white_texture(), rect->position, rect->half_size, 0.0, (vec4){ 0.8, 0.8, 0.8, 1.0 });
	} else 
		gfx_draw_texture_rect(gfx_white_texture(), rect->position, rect->half_size, 0.0, (vec4){ 0.6, 0.6, 0.6, 1.0 });

	gfx_font_size(content_size, FONT_ROBOTO, 1.0, "%s", button->label);
	vec2_sub(content_pos, rect->position, content_size);
	gfx_draw_font2(FONT_ROBOTO, content_pos, 1.0, (vec4){ 0.0, 0.0, 0.0, 1.0 }, "%s", button->label);
}

void 
UI_BUTTON_event(UIObject obj, UIEvent *ev, Rectangle *content)
{
	switch(ev->event_type) {
	case UI_DRAW:
		button_draw(obj, content);
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
