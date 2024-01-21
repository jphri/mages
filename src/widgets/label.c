#include "../graphics.h"
#include "../ui.h"

void
label_draw(UIObject obj, Rectangle *rect)
{
	UI_LABEL_struct *label = ui_data(obj);
	vec2 content_size;
	vec2 content_pos;

	gfx_font_size(content_size, FONT_ROBOTO, 1.0, "%s", label->label_ptr);
	vec2_sub(content_pos, rect->position, content_size);
	gfx_draw_font2(FONT_ROBOTO, content_pos, 1.0, (vec4){ 0.0, 0.0, 0.0, 1.0 }, "%s", label->label_ptr);
}

void
UI_LABEL_event(UIObject label_obj, UIEvent *event, Rectangle *rect) 
{
	if(event->event_type == UI_DRAW)
		label_draw(label_obj, rect);
}
