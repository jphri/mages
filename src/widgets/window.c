#include "../graphics.h"
#include "../ui.h"

#define WINDOW_TITLE_HEIGHT 14

static void window_mouse_button(UIObject *window, UIEvent *event);
static void window_mouse_move(UIObject *window, UIEvent *event);

static void window_draw(UIObject *window, Rectangle *all_window_rect);
//static void window_draw_title(UI_WINDOW_struct *window, Rectangle *title_rect);

#define WINDOW(obj) WIDGET(UI_WINDOW, obj)

UIObject *
ui_window_new(void)
{
	UIObject *window = ui_new_object(0, UI_WINDOW);
	UIObject *title_label = ui_label_new();
	UIObject *title_layout = ui_layout_new();
	UIObject *child_root = ui_new_object(window, UI_ROOT);

	ui_label_set_text(title_label, "Window");
	ui_label_set_color(title_label, (vec4){ 1.0, 1.0, 1.0, 1.0 });

	ui_layout_set_order(title_layout, UI_LAYOUT_HORIZONTAL);
	ui_layout_set_border(title_layout, 0, 0, 5, 5);
	ui_layout_set_background(title_layout, (vec4){ 0.0, 0.0, 0.4, 1.0 });
	ui_child_append(title_layout, title_label);

	ui_child_append(window, title_layout);

	WINDOW(window)->title_label = title_label;
	WINDOW(window)->title_layout = title_layout;
	WINDOW(window)->decorated = true;
	WINDOW(window)->child_root = child_root;

	return window;
}

void
ui_window_set_size(UIObject *window, vec2 size)
{
	UI_WINDOW_struct *wdata = WIDGET(UI_WINDOW, window);
	vec2_dup(wdata->window_rect.half_size, size);
}

void
ui_window_set_position(UIObject *window, UIOrigin origin, vec2 position)
{
	UI_WINDOW_struct *wdata = WIDGET(UI_WINDOW, window);
	vec2_dup(wdata->position.position, position);
	wdata->position.origin = origin;
}

void
ui_window_set_background(UIObject *window, vec4 background)
{
	UI_WINDOW_struct *wdata = WIDGET(UI_WINDOW, window);
	vec4_dup(wdata->background, background);
}

void
ui_window_set_border(UIObject *window, vec2 border)
{
	UI_WINDOW_struct *wdata = WIDGET(UI_WINDOW, window);
	vec2_dup(wdata->border, border);
}

void
ui_window_set_title(UIObject *window, const char *title)
{
	UI_WINDOW_struct *wdata = WIDGET(UI_WINDOW, window);
	ui_label_set_text(wdata->title_label, title);
}

void
UI_WINDOW_event(UIObject *obj, UIEvent *event, Rectangle *rect)
{
	Rectangle all_rect;
	Rectangle title_rect;

	ui_position_translate(&WINDOW(obj)->position, rect, WINDOW(obj)->window_rect.position);
	if(WINDOW(obj)->decorated) {
		// add border
		vec2_dup(all_rect.position, WINDOW(obj)->window_rect.position);
		vec2_add(all_rect.half_size, WINDOW(obj)->window_rect.half_size, WINDOW(obj)->border);

		// add title
		vec2_sub(all_rect.position, all_rect.position, (vec2){ 0, WINDOW_TITLE_HEIGHT });
		vec2_add(all_rect.half_size, all_rect.half_size, (vec2){ 0, WINDOW_TITLE_HEIGHT });

		// make title rect
		title_rect.position[0] = all_rect.position[0];
		title_rect.position[1] = all_rect.position[1] - all_rect.half_size[1] + WINDOW(obj)->border[1] + WINDOW_TITLE_HEIGHT;
		title_rect.half_size[0] = all_rect.half_size[0] - WINDOW(obj)->border[0];
		title_rect.half_size[1] = WINDOW_TITLE_HEIGHT;
	} else {
		vec2_dup(all_rect.position, WINDOW(obj)->window_rect.position);
		vec2_dup(all_rect.half_size, WINDOW(obj)->window_rect.half_size);
	}

	if(event->event_type == UI_DRAW) {
		window_draw(obj, &all_rect);
	}
	(void)title_rect;

	ui_default_mouse_handle(obj, event, &all_rect);

	if(WINDOW(obj)->decorated)
		ui_call_event(WINDOW(obj)->title_layout, event, &title_rect);
	
	if(event->event_type == UI_DRAW)
		gfx_push_clip(WINDOW(obj)->window_rect.position, WINDOW(obj)->window_rect.half_size);

	ui_call_event(WINDOW(obj)->child_root, event, &WINDOW(obj)->window_rect);

	if(event->event_type == UI_DRAW)
		gfx_pop_clip();

	if(WINDOW(obj)->decorated) {
		if(event->event_type == UI_MOUSE_MOTION) {
			window_mouse_move(obj, event);
		} else if(event->event_type == UI_MOUSE_BUTTON) {
			window_mouse_button(obj, event);
		}
	}
}

void
ui_window_append_child(UIObject *window, UIObject *child)
{
	ui_child_append(WINDOW(window)->child_root, child);
}

void
ui_window_append_prepend(UIObject *window, UIObject *child)
{
	ui_child_append(WINDOW(window)->child_root, child);
}

void
ui_window_set_decorated(UIObject *window, bool decorated)
{
	WINDOW(window)->decorated = decorated;
}

void
window_draw(UIObject *window, Rectangle *all_window_rect)
{
	Rectangle rect = *all_window_rect;
	vec2_sub(rect.half_size, rect.half_size, WINDOW(window)->border);

	/* border */
	gfx_draw_texture_rect(gfx_white_texture(), all_window_rect->position, all_window_rect->half_size, 0.0, (vec4){ 0.3, 0.3, 0.3, 1.0 });

	/* content */
	gfx_draw_texture_rect(gfx_white_texture(), rect.position, rect.half_size, 0.0, (vec4){ 0.5, 0.5, 0.5, 1.0 });
}

//void
//window_draw_title(UI_WINDOW_struct *window, Rectangle *title_rect)
//{
//	(void)window;
//	gfx_push_clip(title_rect->position, title_rect->half_size);
//	gfx_draw_texture_rect(gfx_white_texture(), title_rect->position, title_rect->half_size, 0.0, (vec4){ 0.0, 0.0, 0.4, 1.0 });
//	gfx_pop_clip();
//}


void 
window_mouse_button(UIObject *window, UIEvent *event)
{
	if(ui_get_active() == 0) {
		if(event->data.mouse.button == UI_MOUSE_LEFT) {
			if(ui_get_hot() == WINDOW(window)->title_layout && event->data.mouse.state) {
				ui_set_active(WINDOW(window)->title_layout);
				vec2_dup(WINDOW(window)->drag_begin, event->data.mouse.position);
				vec2_dup(WINDOW(window)->drag_begin_pos, WINDOW(window)->position.position);
			}
		}
	} else if(ui_get_active() == WINDOW(window)->title_layout) {
		if(event->data.mouse.button == UI_MOUSE_LEFT) {
			if(!event->data.mouse.state) {
				ui_set_active(0);
			}
		}
	}
}

void
window_mouse_move(UIObject *window, UIEvent *event)
{
	if(ui_get_active() == WINDOW(window)->title_layout) {
		vec2 delta;
		vec2_sub(delta, WINDOW(window)->drag_begin, event->data.mouse.position);
		vec2_sub(WINDOW(window)->position.position, WINDOW(window)->drag_begin_pos, delta);
	}
}
