#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>

#include "../global.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"
#include "../graphics.h"
#include "../util.h"
#include "../map.h"
#include "../ui.h"
#include "editor.h"

typedef void (*ThingRender)(Thing *thing);

static void thing_null_render(Thing *thing);
static void thing_player_render(Thing *thing);
static void thing_dummy_render(Thing *thing);

static void render_thing(Thing *thing);
static void select_thing(vec2 position);
static void update_thing_context(void);

static void thing_type_name_cbk(UIObject obj, void *userptr);

static MouseState mouse_state;
static vec2 move_offset, mouse_position;
static bool ctrl_pressed;

static Thing *selected_thing;
static UIObject thing_context, thing_type_name;

static StrView type_string[LAST_THING];

static ThingRender renders[] = {
	[THING_NULL] = thing_null_render,
	[THING_PLAYER] = thing_player_render,
	[THING_DUMMY] = thing_dummy_render
};

void
thing_init(void)
{
	type_string[THING_NULL]   = to_strview("");
	type_string[THING_PLAYER] = to_strview("THING_PLAYER");
	type_string[THING_DUMMY]  = to_strview("THING_DUMMY");

	thing_context = ui_new_object(UI_ROOT, 0);
	UIObject layout = ui_layout_new();
	ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
	ui_layout_set_border(layout, 2.0, 2.0, 2.0, 2.0);
	ui_layout_set_fixed_size(layout, 15.0); \
	{
		UIObject sublayout, label;

		#define BEGIN_LAYOUT(NAME) \
		sublayout = ui_layout_new(); \
		ui_layout_set_order(sublayout, UI_LAYOUT_HORIZONTAL); \
		label = ui_label_new(); \
		ui_label_set_text(label, NAME); \
		ui_label_set_alignment(label, UI_LABEL_ALIGN_RIGHT); \
		ui_layout_append(sublayout, label);

		#define END_LAYOUT \
		ui_layout_append(layout, sublayout);

		BEGIN_LAYOUT("Thing Type: "); {
			thing_type_name = ui_text_input_new();
			ui_text_input_set_cbk(thing_type_name, NULL, thing_type_name_cbk);
			ui_layout_append(sublayout, thing_type_name);
		} END_LAYOUT;
	}
	ui_child_append(thing_context, layout);
}

void
thing_terminate(void)
{
	ui_del_object(thing_context);
}

void
thing_enter(void)
{
	update_thing_context();
}

void
thing_exit(void)
{
	selected_thing = NULL;
	ui_deparent(thing_context);
}

void thing_render(void)
{
	TextureStamp stamp;

	gfx_draw_begin(NULL);
	for(int k = 0; k < SCENE_LAYERS; k++)
	for(int i = 0; i < editor.map->w * editor.map->h; i++) {
		float x = (     (i % editor.map->w) + 0.5);
		float y = ((int)(i / editor.map->w) + 0.5);

		int spr = editor.map->tiles[i + k * editor.map->w * editor.map->h] - 1;
		if(spr < 0)
			continue;
		int spr_x = spr % 16;
		int spr_y = spr / 16;

		stamp = get_sprite(SPRITE_TERRAIN, spr_x, spr_y);

		gfx_draw_texture_rect(
				&stamp,
				(vec2){ x, y },
				(vec2){ 0.5, 0.5 },
				0.0,
				(vec4){ 1.0, 1.0, 1.0, 1.0 }
		);
	}

	for(Thing *c = editor.map->things; c; c = c->next) {
		render_thing(c);
	}

	gfx_draw_end();
}

void
thing_keyboard(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_LCTRL: ctrl_pressed = true; break;
		case SDLK_DELETE:
			if(selected_thing) {
				if(selected_thing->prev) selected_thing->prev->next = selected_thing->next;
				if(selected_thing->next) selected_thing->next->prev = selected_thing->prev;
				if(selected_thing == editor.map->things)
					editor.map->things = selected_thing->next;
				selected_thing = NULL;
			}
			break;
		}
	} else {
		switch(event->key.keysym.sym) {
		case SDLK_LCTRL: ctrl_pressed = false; break;
		}
	}
}

void
thing_mouse_motion(SDL_Event *event)
{
	vec2 v;
	gfx_pixel_to_world((vec2){ event->button.x, event->button.y }, mouse_position);
	switch(mouse_state) {
	case MOUSE_MOVING:
		vec2_sub(v, move_offset, (vec2){ event->motion.x, event->motion.y });
		editor_move_camera(v);
		move_offset[0] = event->button.x;
		move_offset[1] = event->button.y;
		break;
	case MOUSE_DRAWING:
		vec2_sub(v, move_offset, mouse_position);
		if(selected_thing) {
			vec2_sub(selected_thing->position, selected_thing->position, v);
		}
		vec2_dup(move_offset, mouse_position);
		break;
	default:
		do {} while(0);
	}
}

void
thing_mouse_button(SDL_Event *event)
{
	Thing *thing;
	if(event->type == SDL_MOUSEBUTTONUP) {
		mouse_state = MOUSE_NOTHING;
	}
	gfx_pixel_to_world((vec2){ event->button.x, event->button.y }, mouse_position);

	if(event->type == SDL_MOUSEBUTTONDOWN && mouse_state == MOUSE_NOTHING) {
		switch(event->button.button) {
		case SDL_BUTTON_LEFT:
			vec2_dup(move_offset, mouse_position);
			select_thing(mouse_position);
			mouse_state = MOUSE_DRAWING;
			break;
		case SDL_BUTTON_MIDDLE:
			thing = malloc(sizeof(*thing));
			thing->type = THING_NULL;
			vec2_dup(thing->position, mouse_position);
			thing->prev = NULL;
			thing->next = editor.map->things;
			if(editor.map->things)
				editor.map->things->prev = thing;

			editor.map->things = thing;
			break;
		case SDL_BUTTON_RIGHT:
			move_offset[0] = event->button.x;
			move_offset[1] = event->button.y;
			mouse_state = MOUSE_MOVING;
			break;
		}
	}
}

void
thing_wheel(SDL_Event *event)
{
	if(ctrl_pressed) {
		editor_delta_zoom(event->wheel.y);
	}
}

void
render_thing(Thing *c)
{
	if(renders[c->type])
		renders[c->type](c);
	else
		thing_null_render(c);
}

void
thing_null_render(Thing *c)
{
	gfx_draw_texture_rect(gfx_white_texture(), c->position, (vec2){ 1.0, 1.0 }, 0.0, (vec4){ 1.0, 0.0, 0.0, 1.0 });

	if(c == selected_thing) {
		gfx_draw_texture_rect(gfx_white_texture(), c->position, (vec2){ 1.0, 1.0 }, 0.0, (vec4){ 0.0, 0.0, 1.0, 0.5 });
	}
}

void
thing_player_render(Thing *c)
{
	TextureStamp stamp = get_sprite(SPRITE_ENTITIES, 0, 0);
	gfx_draw_texture_rect(&stamp, c->position, (vec2){ 1.0, 1.0 }, 0.0, (vec4){ 1.0, 1.0, 1.0, 1.0 });

	if(c == selected_thing) {
		gfx_draw_texture_rect(gfx_white_texture(), c->position, (vec2){ 1.0, 1.0 }, 0.0, (vec4){ 0.0, 0.0, 1.0, 0.5 });
	}
}

void
thing_dummy_render(Thing *c)
{
	TextureStamp stamp = get_sprite(SPRITE_ENTITIES, 0, 2);
	gfx_draw_texture_rect(&stamp, c->position, (vec2){ 1.0, 1.0 }, 0.0, (vec4){ 1.0, 1.0, 1.0, 1.0 });

	if(c == selected_thing) {
		gfx_draw_texture_rect(gfx_white_texture(), c->position, (vec2){ 1.0, 1.0 }, 0.0, (vec4){ 0.0, 0.0, 1.0, 0.5 });
	}
}

void
select_thing(vec2 v)
{
	selected_thing = NULL;
	for(Thing *c = editor.map->things; c; c = c->next) {
		Rectangle r = {
			.position = { c->position[0], c->position[1] },
			.half_size = { 1.0, 1.0 }
		};
		if(rect_contains_point(&r, v)) {
			selected_thing = c;
			break;
		}
	}
	update_thing_context();
}

void
update_thing_context(void)
{
	if(selected_thing) {
		ui_text_input_set_text(thing_type_name, type_string[selected_thing->type]);
		ui_window_append_child(editor.context_window, thing_context);
	} else {
		ui_deparent(thing_context);
	}
}

void
thing_type_name_cbk(UIObject obj, void *userptr)
{
	(void)userptr;
	selected_thing->type = THING_NULL;
	StrView v = ui_text_input_get_str(obj);
	
	for(int i = 0; i < LAST_THING; i++) {
		if(strview_cmpstr(v, type_string[i]) == 0) {
			selected_thing->type = i;
			return;
		}
	}
}
