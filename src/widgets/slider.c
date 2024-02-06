#include "../ui.h"
#include "../graphics.h"

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

	return slider;
}

void
ui_slider_set_max_value(UIObject slider, int max_value)
{
	WIDGET(UI_SLIDER, slider)->max_value = max_value;
}

void
ui_slider_set_value(UIObject slider, int value)
{
	UI_SLIDER_struct *sdata = ui_data(slider);
	sdata->value = value;
	if(sdata->old_value != value) {
		sdata->old_value = value;
		if(sdata->cbk)
			sdata->cbk(slider, sdata->user_ptr);
	}
}

int
ui_slider_get_value(UIObject slider)
{
	UI_SLIDER_struct *sdata = ui_data(slider);
	return sdata->value;
}

void
ui_slider_set_callback(UIObject slider, void *userptr, void (*cbk)(UIObject, void *))
{
	UI_SLIDER_struct *sdata = ui_data(slider);
	sdata->user_ptr = userptr;
	sdata->cbk = cbk;
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

		ui_slider_set_value(obj, p * sl->max_value);
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
	ui_default_mouse_handle(obj, ev, rect);
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
	
	slider.width = 6.0;
	slider.border = 10.0;
	slider.slider_border = slider.width;
	slider.slider_half_size = rect->half_size[0] - slider.slider_border;
	slider.perc = ls->value / (float)ls->max_value;

	slider.handle_rect.half_size[0] = slider.width;
	slider.handle_rect.half_size[1] = rect->half_size[1];
	slider.handle_rect.position[0] = rect->position[0] + (slider.perc * 2.0 - 1.0) * slider.slider_half_size;
	slider.handle_rect.position[1] = rect->position[1];

	return slider;
}
