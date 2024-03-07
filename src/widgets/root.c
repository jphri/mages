#include "../ui.h"

void
UI_ROOT_event(UIObject *obj, UIEvent *ev, Rectangle *rect)
{
	switch(ev->event_type) {
	case UI_DRAW:
		/* the only retarded that do this in reverse is the root, lol */
		/* lets laugh at the UI_NULL */
		/* loooooooooooooool */
		for(UIObject *child = ui_last_child(obj); child; child = ui_child_prev(child)) {
			ui_call_event(child, ev, rect);
		}
		break;
	default:
		ui_default_mouse_handle(obj, ev, rect);
		for(UIObject *child = ui_child(obj); child; child = ui_child_next(child)) {
			ui_call_event(child, ev, rect);
		}
		if(ev->event_type == UI_MOUSE_BUTTON || ev->event_type == UI_MOUSE_MOTION) {
			if(ui_get_hot() == obj) {
				ui_set_hot(ui_get_parent(obj));
			}
		}
		break;
	}
}
