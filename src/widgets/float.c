#include "../ui.h"
#include "../graphics.h"

void
UI_FLOAT_event(UIObject obj, UIEvent *ev, Rectangle *rect)
{
	(void)rect;
	UI_FLOAT_struct *fl = ui_data(obj);

	switch(ev->event_type) {
	case UI_DRAW:
		gfx_push_clip(fl->rect.position, fl->rect.half_size);
		goto process_childs;

	case UI_MOUSE_MOTION:
		ui_default_mouse_handle(obj, ev, &fl->rect);
		goto process_childs;
		break;
	case UI_MOUSE_BUTTON:
		goto process_childs;
	}
	return;
	
process_childs:
	for(UIObject child = ui_child(obj); child; child = ui_child_next(child)) {
		ui_call_event(child, ev, &fl->rect);
	}

	if(ev->event_type == UI_DRAW)
		gfx_pop_clip();
}
