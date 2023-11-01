#include <stdio.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "../global.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"
#include "../graphics.h"
#include "../util.h"
#include "../map.h"
#include "../editor.h"

static float zoom = 16.0;
static bool  ctrl_pressed = false;
static vec2  begin_offset, move_offset, offset;
static int current_layer = 0;
static MouseState mouse_state;

void
edit_keyboard(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_1: editor.editor_state = EDITOR_EDIT_MAP; break;
		case SDLK_2: editor.editor_state = EDITOR_SELECT_TILE; break;
		case SDLK_3: editor.editor_state = EDITOR_EDIT_COLLISION; break;
		case SDLK_LCTRL: ctrl_pressed = true; break;
		case SDLK_s:
			if(ctrl_pressed) {
				export_map("exported_map.map");
				printf("Map exported to exported_map.map\n");
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
edit_mouse_motion(SDL_Event *event)
{
	int x, y;
	switch(mouse_state) {
	case MOUSE_MOVING:
		offset[0] = begin_offset[0] + event->motion.x - move_offset[0];
		offset[1] = begin_offset[1] + event->motion.y - move_offset[1];
		break;
	case MOUSE_DRAWING:
		x = ((event->motion.x - offset[0]) / zoom);
		y = ((event->motion.y - offset[1]) / zoom);
		if(x < 0 || x >= editor.map->w || y < 0 || y >= editor.map->h) return;
		editor.map->tiles[x + y * editor.map->w + current_layer * editor.map->w * editor.map->h] = editor.current_tile;
	default:
		do {} while(0);
	}
}

void
edit_mouse_button(SDL_Event *event)
{
	if(event->type == SDL_MOUSEBUTTONUP) {
		mouse_state = MOUSE_NOTHING;
	}
	
	if(event->type == SDL_MOUSEBUTTONDOWN && mouse_state == MOUSE_NOTHING) {
		switch(event->button.button) {
		case SDL_BUTTON_LEFT: mouse_state = MOUSE_DRAWING; break;
		case SDL_BUTTON_RIGHT: 
			move_offset[0] = event->button.x; 
			move_offset[1] = event->button.y; 
			vec2_dup(begin_offset, offset);
			mouse_state = MOUSE_MOVING; 
			break;
		}
	}
}

void
edit_wheel(SDL_Event *event)
{
	if(ctrl_pressed) {
		zoom += event->wheel.y;
		if(zoom < 1.0)
			zoom = 1.0;
	} else {
		current_layer += event->wheel.y;
		if(current_layer < 0)
			current_layer = 0;
		if(current_layer > SCENE_LAYERS)
			current_layer = SCENE_LAYERS;

		printf("Current Layer: %d\n", current_layer);
	}
}

void
edit_render()
{
	gfx_set_camera(offset, (vec2){ zoom, zoom });


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

		gfx_draw_sprite(&(Sprite){
				.position = { x, y },
				.sprite_id = { spr_x, spr_y },
				.color = { 1.0, 1.0, 1.0, 1.0 },
				.half_size = { 0.5, 0.5 },
				.type = editor.map_atlas,
				.clip_region = { 0, 0, 1000, 1000 }
				});
	}

	for(int x = 0; x < editor.map->w + 1; x++) {
		gfx_draw_line(
			TEXTURE_UI,
			(vec2){ (x - 0.1), (0.0) }, 
			(vec2){ (x - 0.1), (editor.map->h) },
			0.1,
			(vec4){ 1.0, 1.0, 1.0, 1.0 },
			(vec4){ 0.0, 0.0, 1000, 1000 }
		);
	}
	for(int y = 0; y < editor.map->h + 1; y++) {
		gfx_draw_line(
			TEXTURE_UI,
			(vec2){ (0.0),           (y - 0.1) }, 
			(vec2){ (editor.map->w), (y - 0.1) },
			0.1,
			(vec4){ 1.0, 1.0, 1.0, 1.0 },
			(vec4){ 0.0, 0.0, 1000, 1000 }
		);
	}
	gfx_draw_end();
}
