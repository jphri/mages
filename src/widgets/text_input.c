#include <ctype.h>

#include "../ui.h"
#include "../graphics.h"

static void tinput_draw(UIObject input, Rectangle *rect);
static void keyboard_event(UIObject obj, UIEvent *event);

UIObject
ui_text_input_new(void)
{
	UIObject obj = ui_new_object(0, UI_TEXT_INPUT);
	arrbuf_init(&WIDGET(UI_TEXT_INPUT, obj)->text_buffer);
	WIDGET(UI_TEXT_INPUT, obj)->carot = 0;
	WIDGET(UI_TEXT_INPUT, obj)->offset = 0;
	ui_text_input_set_filter(obj, isprint);
	return obj;
}

StrView
ui_text_input_get_str(UIObject obj)
{
	void *buffer = WIDGET(UI_TEXT_INPUT, obj)->text_buffer.data;
	size_t size  = WIDGET(UI_TEXT_INPUT, obj)->text_buffer.size;
	return to_strview_buffer(buffer, size);
}

void
ui_text_input_set_cbk(UIObject obj, void *userptr, void (*cbk)(UIObject, void*))
{
	WIDGET(UI_TEXT_INPUT, obj)->cbk = cbk;
	WIDGET(UI_TEXT_INPUT, obj)->userptr = userptr;
}

void
ui_text_input_clear(UIObject obj)
{
	arrbuf_clear(&WIDGET(UI_TEXT_INPUT, obj)->text_buffer);
	WIDGET(UI_TEXT_INPUT, obj)->carot = 0;
	WIDGET(UI_TEXT_INPUT, obj)->offset = 0;
}

void
ui_text_input_set_text(UIObject obj, StrView str)
{
	ui_text_input_clear(obj);
	arrbuf_insert(&WIDGET(UI_TEXT_INPUT, obj)->text_buffer, str.end - str.begin, str.begin);
	WIDGET(UI_TEXT_INPUT, obj)->carot = 0;
}

void 
ui_text_input_set_filter(UIObject obj, int (*filter)(int codepoint))
{
	WIDGET(UI_TEXT_INPUT, obj)->filter = filter;
}

void 
UI_TEXT_INPUT_event(UIObject obj, UIEvent *event, Rectangle *rect)
{
	switch(event->event_type) {
	case UI_DELETE:
		arrbuf_free(&WIDGET(UI_TEXT_INPUT, obj)->text_buffer);
		break;
	case UI_DRAW:
		tinput_draw(obj, rect);
		break;
	case UI_MOUSE_BUTTON:
		ui_default_mouse_handle(obj, event, rect);

		if(ui_get_text_active() == obj) {
			if(ui_get_hot() != obj)
				ui_set_text_active(0);
		} else if(ui_get_text_active() == 0) {
			if(ui_get_hot() == obj)
				ui_set_text_active(obj);
		}

		break;
	case UI_TEXT_ENTRY:
		if(ui_get_text_active() == obj) {
			StrView str = to_strview_buffer(event->data.text.text, event->data.text.text_size);
			int f = WIDGET(UI_TEXT_INPUT, obj)->filter(utf8_decode(str));

			if(!f)
				return;

			arrbuf_insert_at(&WIDGET(UI_TEXT_INPUT, obj)->text_buffer, event->data.text.text_size, event->data.text.text, WIDGET(UI_TEXT_INPUT, obj)->carot);
			WIDGET(UI_TEXT_INPUT, obj)->carot += event->data.text.text_size;

			if(WIDGET(UI_TEXT_INPUT, obj)->cbk)
				WIDGET(UI_TEXT_INPUT, obj)->cbk(obj, WIDGET(UI_TEXT_INPUT, obj)->userptr);
		}
		break;
	case UI_KEYBOARD:
		keyboard_event(obj, event);
		break;
	default:
		break;
	}
}

void 
tinput_draw(UIObject input, Rectangle *rect)
{
	vec2 text_position;
	vec2 caret_position;
	(void)input;

	Span span = arrbuf_span(&WIDGET(UI_TEXT_INPUT, input)->text_buffer);
	StrView view = {
		.begin = span.begin,
		.end   = span.end
	};
	StrView caret_view = {
		.begin = view.begin,
		.end = (const unsigned char *)view.begin + WIDGET(UI_TEXT_INPUT, input)->carot
	};
	
	gfx_font_size_view(caret_position, FONT_ROBOTO, 12/32.0, caret_view);
	vec2_add(caret_position, caret_position, caret_position);
	vec2_sub(text_position, rect->position, rect->half_size);
	caret_position[1] = 6.0;
	vec2_add(caret_position, text_position, caret_position);

	float offset = WIDGET(UI_TEXT_INPUT, input)->offset;
	float caret_x = caret_position[0] - offset;
	float dist = caret_x - rect->position[0];
	if(fabsf(dist) > rect->half_size[0]) {
		float delta = copysignf((fabsf(dist) - rect->half_size[0]), dist);
		WIDGET(UI_TEXT_INPUT, input)->offset += delta;
	}

	caret_position[0] -= WIDGET(UI_TEXT_INPUT, input)->offset;
	text_position[0] -= WIDGET(UI_TEXT_INPUT, input)->offset;

	gfx_push_clip(rect->position, rect->half_size);
	gfx_draw_texture_rect(gfx_white_texture(), rect->position, rect->half_size, 0, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	gfx_draw_font(FONT_ROBOTO, text_position, 12/32.0, (vec4){ 0.0, 0.0, 0.0, 1.0 }, view);
	gfx_draw_texture_rect(gfx_white_texture(), caret_position, (vec2){ 1.0, 6 }, 0.0, (vec4){ 0.0, 0.0, 0.0, 1.0 });
	gfx_pop_clip();
}

void 
keyboard_event(UIObject obj, UIEvent *event)
{
	Span span;
	StrView view;
	ArrayBuffer *buffer;

	int remove_pos, delta;
	if(ui_get_text_active() != obj)
		return;

	span = arrbuf_span(&WIDGET(UI_TEXT_INPUT, obj)->text_buffer);
	view = to_strview_buffer(span.begin, (unsigned char*)span.end - (unsigned char*)span.begin);
	switch(event->data.key) {
	case UI_KEY_LEFT:
		WIDGET(UI_TEXT_INPUT, obj)->carot -= utf8_multibyte_prev(view, WIDGET(UI_TEXT_INPUT, obj)->carot);
		break;
	case UI_KEY_RIGHT:
		WIDGET(UI_TEXT_INPUT, obj)->carot += utf8_multibyte_next(view, WIDGET(UI_TEXT_INPUT, obj)->carot);
		break;
	case UI_KEY_BACKSPACE:
		buffer = &WIDGET(UI_TEXT_INPUT, obj)->text_buffer;
		delta = utf8_multibyte_prev(view, WIDGET(UI_TEXT_INPUT, obj)->carot);
		remove_pos = WIDGET(UI_TEXT_INPUT, obj)->carot - delta;

		arrbuf_remove(buffer, delta, remove_pos);
		WIDGET(UI_TEXT_INPUT, obj)->carot -= delta;

		if(WIDGET(UI_TEXT_INPUT, obj)->cbk)
			WIDGET(UI_TEXT_INPUT, obj)->cbk(obj, WIDGET(UI_TEXT_INPUT, obj)->userptr);

		break;
	default:
		break;
	}
}
