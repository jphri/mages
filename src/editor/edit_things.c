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

static void thing_type_name_cbk(UIObject *obj, void *userptr);
static void thing_float(UIObject *obj, void *userptr);
static void thing_direction(UIObject *obj, void *userptr);

static void update_inputs(void);

static MouseState mouse_state;
static vec2 move_offset, mouse_position;
static bool ctrl_pressed;

static Thing *selected_thing;
static UIObject *thing_context, *thing_type_name;
static UIObject *uiposition_x, *uiposition_y;
static UIObject *uihealth, *uihealth_max;
static UIObject *uidirection;

static ArrayBuffer helper_print;

static StrView type_string[LAST_THING];

static ThingRender renders[] = {
	[THING_NULL] = thing_null_render,
	[THING_PLAYER] = thing_player_render,
	[THING_DUMMY] = thing_dummy_render
};

static const char *direction_str[] = {
	[DIR_UP] = "up",
	[DIR_LEFT] = "left",
	[DIR_DOWN] = "down",
	[DIR_RIGHT] = "right"
};

void
thing_init(void)
{
	arrbuf_init(&helper_print);
	type_string[THING_NULL]   = to_strview("");
	type_string[THING_PLAYER] = to_strview("THING_PLAYER");
	type_string[THING_DUMMY]  = to_strview("THING_DUMMY");

	thing_context = ui_new_object(0, UI_ROOT);
	UIObject *layout = ui_layout_new();
	ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
	ui_layout_set_border(layout, 2.0, 2.0, 2.0, 2.0);
	ui_layout_set_fixed_size(layout, 10.0); \
	{
		UIObject *sublayout, *label;

		#define BEGIN_LAYOUT(NAME) \
		sublayout = ui_layout_new(); \
		ui_layout_set_order(sublayout, UI_LAYOUT_HORIZONTAL); \
		ui_layout_set_border(sublayout, 2.0, 2.0, 2.0, 2.0); \
		label = ui_label_new(); \
		ui_label_set_text(label, NAME); \
		ui_label_set_alignment(label, UI_LABEL_ALIGN_LEFT); \
		ui_layout_append(sublayout, label);

		#define END_LAYOUT \
		ui_layout_append(layout, sublayout);


		BEGIN_LAYOUT("type"); {
			thing_type_name = ui_text_input_new();
			ui_text_input_set_cbk(thing_type_name, NULL, thing_type_name_cbk);
			ui_layout_append(sublayout, thing_type_name);
		} END_LAYOUT;

		BEGIN_LAYOUT("position"); {
			UIObject *retarded = ui_layout_new(); 
			ui_layout_set_order(retarded, UI_LAYOUT_HORIZONTAL);
			{
				uiposition_x = ui_text_input_new();
				ui_text_input_set_cbk(uiposition_x, (void*)(offsetof(Thing, position[0])), thing_float);
				ui_layout_append(retarded, uiposition_x);

				uiposition_y = ui_text_input_new();
				ui_text_input_set_cbk(uiposition_y, (void*)(offsetof(Thing, position[1])), thing_float);
				ui_layout_append(retarded, uiposition_y);
			}
			ui_layout_append(sublayout, retarded);
		} END_LAYOUT;

		BEGIN_LAYOUT("health"); {
			UIObject *retarded = ui_layout_new(); 
			ui_layout_set_order(retarded, UI_LAYOUT_HORIZONTAL);
			{
				uihealth = ui_text_input_new();
				ui_text_input_set_cbk(uihealth, (void*)(offsetof(Thing, health)), thing_float);
				ui_layout_append(retarded, uihealth);
			}
			ui_layout_append(sublayout, retarded);
		} END_LAYOUT;

		BEGIN_LAYOUT("health_max"); {
			UIObject *retarded = ui_layout_new(); 
			ui_layout_set_order(retarded, UI_LAYOUT_HORIZONTAL);
			{
				uihealth_max = ui_text_input_new();
				ui_text_input_set_cbk(uihealth_max, (void*)(offsetof(Thing, health_max)), thing_float);
				ui_layout_append(retarded, uihealth_max);
			}
			ui_layout_append(sublayout, retarded);
		} END_LAYOUT;

		BEGIN_LAYOUT("direction"); {
			UIObject *retarded = ui_layout_new(); 
			ui_layout_set_order(retarded, UI_LAYOUT_HORIZONTAL);
			{
				uidirection = ui_text_input_new();
				ui_text_input_set_cbk(uidirection, (void*)(offsetof(Thing, direction)), thing_direction);
				ui_layout_append(retarded, uidirection);
			}
			ui_layout_append(sublayout, retarded);
		} END_LAYOUT;
	}
	ui_child_append(thing_context, layout);
}

void
thing_terminate(void)
{
	arrbuf_free(&helper_print);
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
	gfx_draw_begin(NULL);
	common_draw_map(SCENE_LAYERS, 1.0);

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
			update_inputs();
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
	gfx_draw_texture_rect(gfx_white_texture(), c->position, (vec2){ 0.5, 0.5 }, 0.0, (vec4){ 1.0, 0.0, 0.0, 1.0 });

	if(c == selected_thing) {
		gfx_draw_texture_rect(gfx_white_texture(), c->position, (vec2){ 0.5, 0.5 }, 0.0, (vec4){ 0.0, 0.0, 1.0, 0.5 });
	}
}

void
thing_player_render(Thing *c)
{
	TextureStamp stamp = get_sprite(SPRITE_ENTITIES, 0, 0);
	gfx_draw_texture_rect(&stamp, c->position, (vec2){ 0.5, 0.5 }, 0.0, (vec4){ 1.0, 1.0, 1.0, 1.0 });

	if(c == selected_thing) {
		gfx_draw_texture_rect(gfx_white_texture(), c->position, (vec2){ 0.5, 0.5 }, 0.0, (vec4){ 0.0, 0.0, 1.0, 0.5 });
	}
}

void
thing_dummy_render(Thing *c)
{
	TextureStamp stamp = get_sprite(SPRITE_ENTITIES, 0, 2);
	gfx_draw_texture_rect(&stamp, c->position, (vec2){ 0.5, 0.5 }, 0.0, (vec4){ 1.0, 1.0, 1.0, 1.0 });

	if(c == selected_thing) {
		gfx_draw_texture_rect(gfx_white_texture(), c->position, (vec2){ 0.5, 0.5 }, 0.0, (vec4){ 0.0, 0.0, 1.0, 0.5 });
	}
}

void
select_thing(vec2 v)
{
	selected_thing = NULL;
	for(Thing *c = editor.map->things; c; c = c->next) {
		Rectangle r = {
			.position = { c->position[0], c->position[1] },
			.half_size = { 0.5, 0.5 }
		};
		if(rect_contains_point(&r, v)) {
			selected_thing = c;
			update_inputs();
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
		update_inputs();
	} else {
		ui_deparent(thing_context);
	}
}

void
thing_type_name_cbk(UIObject *obj, void *userptr)
{
	(void)userptr;
	selected_thing->type = THING_NULL;
	StrView v = ui_text_input_get_str(obj);
	
	for(int i = 0; i < LAST_THING; i++) {
		if(strview_cmpstr(v, type_string[i]) == 0) {
			selected_thing->type = i;
			update_inputs();
			return;
		}
	}
}

void
thing_float(UIObject *obj, void *userptr)
{
	float *ptr = (void*)((uintptr_t)selected_thing + (uintptr_t)userptr);
	strview_float(ui_text_input_get_str(obj), ptr);
}

void
thing_direction(UIObject *obj, void *userptr)
{
	Direction *dir = (void*)((uintptr_t)selected_thing + (uintptr_t)userptr);
	for(size_t i = 0; i < LENGTH(direction_str); i++) {
		if(strview_cmp(ui_text_input_get_str(obj), direction_str[i]) == 0) {
			*dir = i;
			return;
		}
	}
}

void
update_inputs(void)
{
	#define SETINPUT(INPUT, FORMAT, COMPONENT) \
	arrbuf_clear(&helper_print); \
	arrbuf_printf(&helper_print, FORMAT, COMPONENT); \
	ui_text_input_set_text(INPUT, to_strview_buffer(helper_print.data, helper_print.size));
	
	SETINPUT(uiposition_x, "%0.2f", selected_thing->position[0]);
	SETINPUT(uiposition_y, "%0.2f", selected_thing->position[1]);
	SETINPUT(uihealth, "%0.2f", selected_thing->health);
	SETINPUT(uihealth_max, "%0.2f", selected_thing->health_max);

	if(selected_thing->direction >= 0 && selected_thing->direction <= 3) {
		SETINPUT(uidirection, "%s", direction_str[selected_thing->direction]);
	} else {
		SETINPUT(uidirection, "%s", "");
	}
}


