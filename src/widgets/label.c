#include "../graphics.h"
#include "../ui.h"

UIObject *
ui_label_new(void)
{
	UIObject *label = ui_new_object(0, UI_LABEL);
	ui_label_set_text(label, "");
	ui_label_set_color(label, (vec4){ 0.0, 0.0, 0.0, 1.0 });

	return label;
}

void
ui_label_set_text(UIObject *object, const char *text_ptr)
{
	UI_LABEL_struct *l = WIDGET(UI_LABEL, object);
	l->label_ptr = text_ptr;
}

void
ui_label_set_color(UIObject *object, vec4 color)
{
	UI_LABEL_struct *l = WIDGET(UI_LABEL, object);
	vec4_dup(l->color, color);
}

void
ui_label_set_border(UIObject *object, vec2 border)
{
	UI_LABEL_struct *l = WIDGET(UI_LABEL, object);
	vec2_dup(l->border, border);
}

void
ui_label_set_alignment(UIObject *object, UILabelAlign align)
{
	UI_LABEL_struct *l = WIDGET(UI_LABEL, object);
	l->align = align;
}

void
label_draw(UIObject *obj, Rectangle *rect)
{
	UI_LABEL_struct *label = WIDGET(UI_LABEL, obj);
	vec2 content_size;
	vec2 content_pos;

	gfx_font_size(content_size, FONT_ROBOTO, 12.0/32.0, "%s", label->label_ptr);

	switch(label->align) {
	case UI_LABEL_ALIGN_LEFT: 
		content_pos[1] = rect->position[1] -  content_size[1];
		content_pos[0] = rect->position[0] - rect->half_size[0];
		break;
	case UI_LABEL_ALIGN_CENTER: 
		vec2_sub(content_pos, rect->position, content_size); 
		break;
	case UI_LABEL_ALIGN_RIGHT:
		content_pos[1] = rect->position[1] -  content_size[1];
		content_pos[0] = rect->position[0] + rect->half_size[0] - content_size[0] * 2;
		break;
	}
	gfx_draw_font2(FONT_ROBOTO, content_pos, 12.0/32.0, label->color, "%s", label->label_ptr);
}

void
UI_LABEL_event(UIObject *label_obj, UIEvent *event, Rectangle *rect) 
{
	if(event->event_type == UI_DRAW)
		label_draw(label_obj, rect);
}
