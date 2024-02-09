#include "../ui.h"
#include "../graphics.h"
#include <stdio.h>

typedef struct {
	float width, border;
	float slider_border;
	float slider_half_size;
	float perc;

	Rectangle handle_rect;
} SliderInfo;

static SliderInfo slider_info(UIObject obj, Rectangle *rect);

UIObject 
ui_slider_new(void)
{
	UIObject slider =  ui_new_object(0, UI_SLIDER);
	WIDGET(UI_SLIDER, slider)->max_value = 1;
	WIDGET(UI_SLIDER, slider)->value = 0;
	WIDGET(UI_SLIDER, slider)->old_value = 0;
	WIDGET(UI_SLIDER, slider)->cbk = NULL;
	WIDGET(UI_SLIDER, slider)->user_ptr = NULL;
	WIDGET(UI_SLIDER, slider)->max = 1.0;
	WIDGET(UI_SLIDER, slider)->min = 0.0;
	WIDGET(UI_SLIDER, slider)->max_value = 64;
	WIDGET(UI_SLIDER, slider)->handle_size = 5.0;
	return slider;
}

void
ui_slider_set_precision(UIObject slider, int max_value)
{
	WIDGET(UI_SLIDER, slider)->max_value = max_value;
}

void
ui_slider_set_handle_size(UIObject object, float size)
{
	WIDGET(UI_SLIDER, object)->handle_size = size;
}

void
ui_slider_set_value_raw(UIObject slider, int value)
{
	UI_SLIDER_struct *sdata = ui_data(slider);
	
	sdata->value = value;
	if(sdata->old_value != value) {
		sdata->old_value = value;
		if(sdata->cbk)
			sdata->cbk(slider, sdata->user_ptr);
	}
}

void
ui_slider_set_value(UIObject slider, float value)
{
	UI_SLIDER_struct *sdata = ui_data(slider);
	float v = (value + sdata->min) * sdata->max_value / (sdata->max - sdata->min);
	ui_slider_set_value_raw(slider, v);
}

float
ui_slider_get_value(UIObject slider)
{
	UI_SLIDER_struct *sdata = ui_data(slider);
	float delta = sdata->max - sdata->min;
	return sdata->value * delta / sdata->max_value + sdata->min;
}

void
ui_slider_set_max_value(UIObject slider, float max)
{
	UI_SLIDER_struct *sdata = ui_data(slider);
	sdata->max = max;
}

void
ui_slider_set_min_value(UIObject slider, float v)
{
	UI_SLIDER_struct *sdata = ui_data(slider);
	sdata->min = v;
}

void
ui_slider_set_callback(UIObject slider, void *userptr, void (*cbk)(UIObject, void *))
{
	UI_SLIDER_struct *sdata = ui_data(slider);
	sdata->user_ptr = userptr;
	sdata->cbk = cbk;
}

void
ui_slider_set_vertical(UIObject slider, bool v)
{
	WIDGET(UI_SLIDER, slider)->vertical = v;
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
	vec2 label_position;

	state = 0;
	if(ui_get_active() == obj)
		state = 2;
	else if(ui_get_hot() == obj)
		state = 1;

	gfx_draw_texture_rect(gfx_white_texture(), rect->position, rect->half_size, 0.0, (vec4){ 0.2, 0.2, 0.2, 1.0 });
	gfx_draw_texture_rect(
		gfx_white_texture(), 
		info.handle_rect.position, 
		info.handle_rect.half_size, 
		0.0, 
		colors[state]
	);
	gfx_font_size(label_position, FONT_ROBOTO, 12/32.0, "%0.2f", ui_slider_get_value(obj));
	vec2_sub(label_position, rect->position, label_position);

	gfx_draw_font2(FONT_ROBOTO, label_position, 12/32.0, (vec4){ 1.0, 1.0, 1.0, 1.0 }, "%0.2f", ui_slider_get_value(obj));
}

void
slider_motion(UIObject obj, UIEvent *ev, Rectangle *rect)
{
	SliderInfo info = slider_info(obj, rect);
	Rectangle handle_fix;

	int axis = WIDGET(UI_SLIDER, obj)->vertical;

	rect_intersect(&handle_fix, rect, &info.handle_rect);
	
	if(ui_get_active() == obj) {
		float p = ev->data.mouse.position[axis] - rect->position[axis];
			  p *= WIDGET(UI_SLIDER, obj)->max_value;
			  p /= info.slider_half_size;
			  p += WIDGET(UI_SLIDER, obj)->max_value;
			  p /= 2;
		
		if(p < 0)
			p = 0;
		if(p > WIDGET(UI_SLIDER, obj)->max_value)
		 	p = WIDGET(UI_SLIDER, obj)->max_value;

		ui_slider_set_value_raw(obj, roundf(p));
	}
}

void
slider_button(UIObject obj, UIEvent *ev, Rectangle *rect)
{
	(void)rect;
	if(ui_get_active() == 0) {
		if(ui_get_hot() == obj) {
			if(ev->data.mouse.button == UI_MOUSE_LEFT && ev->data.mouse.state) {
				ui_set_active(obj);
				slider_motion(obj, ev, rect);
			}
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
	ui_default_mouse_handle(obj, ev, rect);
	ui_body_publish(rect);
	switch(ev->event_type) {
	case UI_DRAW:
		slider_draw(obj, rect);
		break;
	case UI_MOUSE_MOTION:
		slider_motion(obj, ev, rect);
		break;
	case UI_MOUSE_BUTTON:
		slider_button(obj, ev, rect);
	default:
		break;
	}
}

static SliderInfo slider_info(UIObject obj, Rectangle *rect)
{
	SliderInfo slider;
	UI_SLIDER_struct *ls = ui_data(obj);

	if(WIDGET(UI_SLIDER, obj)->handle_size < 0) {
		slider.width = rect->half_size[0] / (WIDGET(UI_SLIDER, obj)->max_value + 1);
	} else {
		slider.width = WIDGET(UI_SLIDER, obj)->handle_size;
	}
	
	slider.border = 10.0;
	slider.slider_border = slider.width;
	slider.perc = ls->value / (float)ls->max_value;

	int axis = WIDGET(UI_SLIDER, obj)->vertical,
	    anti_axis = 1 - axis;

	slider.slider_half_size = rect->half_size[axis] - slider.slider_border;
	slider.handle_rect.half_size[axis] = slider.width;
	slider.handle_rect.half_size[anti_axis] = rect->half_size[anti_axis];
	slider.handle_rect.position[axis] = rect->position[axis] + (slider.perc * 2.0 - 1.0) * slider.slider_half_size;
	slider.handle_rect.position[anti_axis] = rect->position[anti_axis];

	return slider;
}
