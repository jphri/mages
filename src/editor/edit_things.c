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

static void render_thing(Thing *thing);
static void select_thing(vec2 position);

static MouseState mouse_state;
static vec2 move_offset, mouse_position;
static bool ctrl_pressed;


static Thing *selected_thing;

void
thing_init(void)
{
}

void
thing_terminate(void)
{
}

void
thing_enter(void)
{
}

void
thing_exit(void)
{
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
	gfx_draw_texture_rect(gfx_white_texture(), c->position, (vec2){ 1.0, 1.0 }, 0.0, (vec4){ 1.0, 0.0, 0.0, 1.0 });
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
		printf("%f %f %f %f\n", c->position[0], c->position[1], v[0], v[1]);
		if(rect_contains_point(&r, v)) {
			selected_thing = c;
			printf("Found something!\n");
			return;
		}
	}
	printf("Found nothing :\\\n");
}