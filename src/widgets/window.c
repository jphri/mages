#include "../graphics.h"
#include "../ui.h"

#define WINDOW_TITLE_HEIGHT 14

void
window_draw(UI_WINDOW_struct *window, Rectangle *all_window_rect)
{
	Rectangle rect = *all_window_rect;
	vec2_sub(rect.half_size, rect.half_size, window->border);

	/* border */
	gfx_draw_texture_rect(gfx_white_texture(), all_window_rect->position, all_window_rect->half_size, 0.0, (vec4){ 0.3, 0.3, 0.3, 1.0 });

	/* content */
	gfx_draw_texture_rect(gfx_white_texture(), rect.position, rect.half_size, 0.0, (vec4){ 0.5, 0.5, 0.5, 1.0 });
}

void
window_draw_title(UI_WINDOW_struct *window, Rectangle *title_rect)
{
	vec2 title_size;
	vec2 title_pos;

	gfx_font_size(title_size, FONT_ROBOTO, 0.5, "%s", window->title);

	title_pos[0] = title_rect->position[0] - title_rect->half_size[0] + 5.0;
	title_pos[1] = title_rect->position[1] - title_rect->half_size[1] + title_size[1];


	gfx_push_clip(title_rect->position, title_rect->half_size);
	gfx_draw_texture_rect(gfx_white_texture(), title_rect->position, title_rect->half_size, 0.0, (vec4){ 0.0, 0.0, 0.4, 1.0 });
	gfx_draw_font2(FONT_ROBOTO, title_pos, 0.5, (vec4){ 1.0, 1.0, 1.0, 1.0 }, "%s", window->title);
	gfx_pop_clip();
}

void
UI_WINDOW_event(UIObject obj, UIEvent *event, Rectangle *rect)
{
	UI_WINDOW_struct *window = ui_data(obj);
	Rectangle all_rect;
	Rectangle title_rect;

	(void)rect;

	// add border
	vec2_dup(all_rect.position, window->window_rect.position);
	vec2_add(all_rect.half_size, window->window_rect.half_size, window->border);

	// add title
	vec2_sub(all_rect.position, all_rect.position, (vec2){ 0, WINDOW_TITLE_HEIGHT });
	vec2_add(all_rect.half_size, all_rect.half_size, (vec2){ 0, WINDOW_TITLE_HEIGHT });

	// make title rect
	title_rect.position[0] = all_rect.position[0];
	title_rect.position[1] = all_rect.position[1] - all_rect.half_size[1] + window->border[1] + WINDOW_TITLE_HEIGHT;
	title_rect.half_size[0] = all_rect.half_size[0] - window->border[0];
	title_rect.half_size[1] = WINDOW_TITLE_HEIGHT;

	switch(event->event_type) {
	case UI_DRAW:
		window_draw(window, &all_rect);
		window_draw_title(window, &title_rect);
		gfx_push_clip(window->window_rect.position, window->window_rect.half_size);
		/* fallthrough */

	case UI_MOUSE_MOTION:
		ui_default_mouse_handle(obj, event, rect);
		if(ui_get_hot() != obj)
			return;

		/* fallthrough */
	default: 
		for(UIObject child = ui_child(obj); child; child = ui_child_next(child)) {
			ui_call_event(child, event, &window->window_rect);
		}

		if(event->event_type == UI_DRAW)
			gfx_pop_clip();

		break;
	}
}
