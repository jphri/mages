#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "../global.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"
#include "../graphics.h"
#include "../util.h"
#include "../map.h"
#include "editor.h"

static vec2 edit_offset = {0}, edit_move_offset = {0}, edit_begin_offset = {0};
static float edit_zoom = 16;
static bool ctrl_pressed = false;

static MouseState edit_mouse_state;

void
select_tile_render(void)
{
	gfx_set_camera(edit_offset, (vec2){ edit_zoom, edit_zoom });

	gfx_draw_begin(NULL);
	for(int i = 0; i < 16 * 16; i++) {
		float x = (     (i % editor.map->w) + 0.5);
		float y = ((int)(i / editor.map->w) + 0.5);

		int spr = i - 1;
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
	for(int x = 0; x < 17; x++) {
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

void
select_tile_keyboard(SDL_Event *event)
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
select_tile_wheel(SDL_Event *event)
{
	edit_zoom += event->wheel.y;
	if(edit_zoom < 1.0)
		edit_zoom = 1.0;
}

void
select_tile_mouse_motion(SDL_Event *event)
{
	int x, y;
	if(editor.editor_state == EDITOR_SELECT_TILE) {
		switch(edit_mouse_state) {
		case MOUSE_MOVING:
			edit_offset[0] = edit_begin_offset[0] + event->motion.x - edit_move_offset[0];
			edit_offset[1] = edit_begin_offset[1] + event->motion.y - edit_move_offset[1];
			break;
		case MOUSE_DRAWING:
			x = ((event->motion.x - edit_offset[0]) / edit_zoom);
			y = ((event->motion.y - edit_offset[1]) / edit_zoom);
			if(x < 0 || x >= 16 || y < 0 || y >= 16) return;
			editor.current_tile = x + y * 16;
		default:
			do {} while(0);
		}
	}
}

void
select_tile_mouse_button(SDL_Event *event)
{
	int x, y;
	if(event->type == SDL_MOUSEBUTTONUP) {
		edit_mouse_state = MOUSE_NOTHING;
	}
	if(event->type == SDL_MOUSEBUTTONDOWN) {
		switch(event->button.button) {
		case SDL_BUTTON_LEFT:
			x = ((event->motion.x - edit_offset[0]) / edit_zoom);
			y = ((event->motion.y - edit_offset[1]) / edit_zoom);
			if(x < 0 || x >= editor.map->w || y < 0 || y >= editor.map->h) return;
			editor.current_tile = x + y * 16;
			printf("Tile Selected: %d\n", editor.current_tile);
			break;
		case SDL_BUTTON_RIGHT: 
			edit_move_offset[0] = event->button.x; 
			edit_move_offset[1] = event->button.y; 
			vec2_dup(edit_begin_offset, edit_offset);
			edit_mouse_state = MOUSE_MOVING; 
			break;
		}
	}
}
