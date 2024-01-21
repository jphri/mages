#include "../graphics.h"
#include "../ui.h"

void
UI_LAYOUT_event(UIObject layout_obj, UIEvent *event, Rectangle *content)
{
	UI_LAYOUT_struct *l = ui_data(layout_obj);
	int child_count = ui_count_child(layout_obj);

	if(child_count == 0)
		return;

	vec2 delta;
	Rectangle rec;

	switch(l->order) {
	case UI_LAYOUT_HORIZONTAL:
		delta[0] = 2 * content->half_size[0] / child_count;
		delta[1] = 0.0;
		rec.half_size[0] = delta[0] * 0.5;
		rec.half_size[1] = content->half_size[1];
		break;
	case UI_LAYOUT_VERTICAL:
		delta[0] = 0.0;
		delta[1] = 2 * content->half_size[1] / child_count;
		rec.half_size[1] = delta[1] * 0.5;
		rec.half_size[0] = content->half_size[0];
		break;
	default:
		die("something is really wrong\n");
		return;
	}
	vec2_sub_scaled(rec.position, content->position, delta, (child_count - 1) / 2.0);
	
	if(event->event_type == UI_MOUSE_MOTION) {
		ui_default_mouse_handle(layout_obj, event, content);
	}

	for(UIObject child = ui_child(layout_obj); child; child = ui_child_next(child)) {
		if(event->event_type == UI_DRAW)
			gfx_push_clip(rec.position, rec.half_size);

		ui_call_event(child, event, &rec);
		vec2_add_scaled(rec.position, rec.position, delta, 1.0);

		if(event->event_type == UI_DRAW)
			gfx_pop_clip();
	}
}
