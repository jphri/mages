#include "../ui.h"
#include "../graphics.h"

typedef struct {
	float width, border;
	float slider_border;
	float slider_half_size;
	float perc;

	Rectangle handle_rect;
} SliderInfo;

static SliderInfo slider_info(UIObject obj, Rectangle *rect)
{
	SliderInfo slider;
	UI_SLIDER_struct *ls = ui_data(obj);
	
	slider.width = 5.0;
	slider.border = 10.0;
	slider.slider_border = slider.width + slider.border;
	slider.slider_half_size = rect->half_size[0] - slider.slider_border;
	slider.perc = ls->value / (float)ls->max_value;

	slider.handle_rect.half_size[0] = slider.width;
	slider.handle_rect.half_size[1] = 10;
	slider.handle_rect.position[0] = rect->position[0] + (slider.perc * 2.0 - 1.0) * slider.slider_half_size;
	slider.handle_rect.position[1] = rect->position[1];

	return slider;
}

static void
slider_draw(UIObject obj, Rectangle *rect)
{
	static vec4 colors[] = {
		{ 0.6, 0.6, 0.6, 1.0 },
		{ 0.8, 0.8, 0.8, 1.0 },
		{ 0.4, 0.4, 0.4, 1.0 }
	};
	SliderInfo info = slider_info(obj, rect);
	int state;

	vec2 p1 = {
		rect->position[0] - info.slider_half_size,
		rect->position[1]
	};
	vec2 p2 = {
		rect->position[0] + info.slider_half_size,
		rect->position[1]
	};

	state = 0;
	if(ui_get_active() == obj)
		state = 2;
	else if(ui_get_hot() == obj)
		state = 1;

	gfx_draw_line(p1, p2, 1.0, (vec4){ 0.0, 0.0, 0.0, 1.0 });
	gfx_draw_texture_rect(
		gfx_white_texture(), 
		info.handle_rect.position, 
		info.handle_rect.half_size, 
		0.0, 
		colors[state]
	);
}

void
slider_motion(UIObject obj, UIEvent *ev, Rectangle *rect)
{
	UI_SLIDER_struct *sl = ui_data(obj);
	SliderInfo info = slider_info(obj, rect);
	Rectangle handle_fix;

	rect_intersect(&handle_fix, rect, &info.handle_rect);
	
	if(ui_get_active() == 0) {
		if(ui_get_hot() == obj) {
			if(!rect_contains_point(&handle_fix, ev->data.mouse.position)) {
				ui_set_hot(0);
			}
		} else {
			if(rect_contains_point(&handle_fix, ev->data.mouse.position)) {
				ui_set_hot(obj);
			}
		}
	} 

	if(ui_get_active() == obj) {
		float p = ev->data.mouse.position[0] - rect->position[0];
			  p /= info.slider_half_size;
			  p += 1.0;
			  p /= 2.0;
		
		if(p < 0)
			p = 0;
		if(p > 1)
		 	p = 1;

		sl->value = p * sl->max_value;
	}
}

void
slider_button(UIObject obj, UIEvent *ev, Rectangle *rect)
{
	(void)rect;
	if(ui_get_active() == 0) {
		if(ui_get_hot() == obj) {
			if(ev->data.mouse.button == UI_MOUSE_LEFT && ev->data.mouse.state)
				ui_set_active(obj);
		}
	}

	if(ui_get_active() == obj) {
		if(ev->data.mouse.button == UI_MOUSE_LEFT && !ev->data.mouse.state) {
			ui_set_active(0);
		}
	}

}

void
UI_SLIDER_event(UIObject obj, UIEvent *ev, Rectangle *rect)
{
	switch(ev->event_type) {
	case UI_DRAW:
		slider_draw(obj, rect);
		break;
	case UI_MOUSE_MOTION:
		slider_motion(obj, ev, rect);
		break;
	case UI_MOUSE_BUTTON:
		slider_button(obj, ev, rect);
	}
}
