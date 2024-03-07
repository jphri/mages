#include "../graphics.h"
#include "../ui.h"

UIObject *
ui_layout_new(void)
{
	UIObject *layout = ui_new_object(0, UI_LAYOUT);
	ui_layout_set_fixed_size(layout, UI_LAYOUT_RELATIVE);
	return layout;
}

void
ui_layout_set_order(UIObject *layout, UILayoutOrder order)
{
	UI_LAYOUT_struct *l = WIDGET(UI_LAYOUT, layout);
	l->order = order;
}

void
ui_layout_set_border(UIObject *layout, float left, float right, float up, float down)
{
	UI_LAYOUT_struct *l = WIDGET(UI_LAYOUT, layout);
	l->border[0] = left;
	l->border[1] = right;
	l->border[2] = up;
	l->border[3] = down;
}

void
ui_layout_append(UIObject *layout, UIObject *obj)
{
	ui_child_append(layout, obj);
}

void
ui_layout_set_background(UIObject *layout, vec4 v)
{
	UI_LAYOUT_struct *l = WIDGET(UI_LAYOUT, layout);
	vec4_dup(l->background, v);
}

void
UI_LAYOUT_event(UIObject *layout_obj, UIEvent *event, Rectangle *bound)
{
	vec2 cont_min, cont_max;
	Rectangle content;
	vec2 delta;
	Rectangle rec;

	UI_LAYOUT_struct *l = WIDGET(UI_LAYOUT, layout_obj);
	int child_count = ui_count_child(layout_obj);

	if(child_count == 0)
		return;

	rect_boundaries(cont_min, cont_max, bound);
	cont_min[0] += l->border[0];
	cont_max[0] -= l->border[1];
	cont_min[1] += l->border[2];
	cont_max[1] -= l->border[3];
	content = rect_from_boundaries(cont_min, cont_max);

	switch(l->order) {
	case UI_LAYOUT_HORIZONTAL:
		delta[0] = WIDGET(UI_LAYOUT, layout_obj)->fixed_size > 0.0 ? WIDGET(UI_LAYOUT, layout_obj)->fixed_size : content.half_size[0] / child_count;
		delta[1] = 0.0;
		rec.half_size[0] = delta[0];
		rec.half_size[1] = content.half_size[1];
		rec.position[0] = content.position[0] - content.half_size[0] + delta[0];
		rec.position[1] = content.position[1];
		break;
	case UI_LAYOUT_VERTICAL:
		delta[0] = 0.0;
		delta[1] = WIDGET(UI_LAYOUT, layout_obj)->fixed_size > 0.0 ? WIDGET(UI_LAYOUT, layout_obj)->fixed_size : content.half_size[1] / child_count;
		rec.half_size[1] = delta[1];
		rec.half_size[0] = content.half_size[0];
		rec.position[0] = content.position[0];
		rec.position[1] = content.position[1] - content.half_size[1] + delta[1];
		break;
	default:
		die("something is really wrong\n");
		return;
	}
	ui_default_mouse_handle(layout_obj, event, bound);

	for(UIObject *child = ui_child(layout_obj); child; child = ui_child_next(child)) {
		if(event->event_type == UI_DRAW) {
			gfx_draw_texture_rect(gfx_white_texture(), bound->position, bound->half_size, 0.0, l->background);
			gfx_push_clip(rec.position, rec.half_size);
		}

		ui_call_event(child, event, &rec);
		vec2_add_scaled(rec.position, rec.position, delta, 2.0);

		if(event->event_type == UI_DRAW)
			gfx_pop_clip();
	}
}

void
ui_layout_set_fixed_size(UIObject *layout, float size)
{
	WIDGET(UI_LAYOUT, layout)->fixed_size = size;
}
