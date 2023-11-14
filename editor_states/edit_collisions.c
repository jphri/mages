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
#include "../editor.h"

static float zoom = 16.0;
static bool  ctrl_pressed = false;
static vec2  begin_offset, move_offset, offset;
static int current_layer = 0;
static MouseState mouse_state;
static CollisionData current_collision;

void
collision_keyboard(SDL_Event *event)
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
collision_mouse_motion(SDL_Event *event)
{
	vec2 full_size, begin_position;
	vec2 v;

	switch(mouse_state) {
	case MOUSE_MOVING:
		offset[0] = begin_offset[0] + event->motion.x - move_offset[0];
		offset[1] = begin_offset[1] + event->motion.y - move_offset[1];
		break;
	case MOUSE_DRAWING:
		gfx_pixel_to_world((vec2){ event->motion.x, event->motion.y }, v);

		vec2_sub(full_size, v, begin_offset);
		vec2_div(current_collision.half_size, full_size, (vec2){ 2.0, 2.0 });
		current_collision.half_size[0] = fabs(current_collision.half_size[0]);
		current_collision.half_size[1] = fabs(current_collision.half_size[1]);
		
		if(full_size[0] > 0)
			begin_position[0] = begin_offset[0];
		else
			begin_position[0] = begin_offset[0] + full_size[0];

		if(full_size[1] > 0)
			begin_position[1] = begin_offset[1];
		else
			begin_position[1] = begin_offset[1] + full_size[1];
		
		vec2_add(current_collision.position, begin_position, current_collision.half_size);
		break;
	default:
		do {} while(0);
	}
}

void
collision_mouse_button(SDL_Event *event)
{
	vec2 pos;
	if(event->type == SDL_MOUSEBUTTONUP) {
		if(mouse_state == MOUSE_DRAWING) {
			CollisionData *data = malloc(sizeof(*data));
			*data = current_collision;
			data->next = editor.map->collision;
			editor.map->collision = data;

			printf("new collision at %f %f %f %f\n",
					data->position[0],
					data->position[1],
					data->half_size[0],
					data->half_size[1]);
		}
		mouse_state = MOUSE_NOTHING;
	}
	
	if(event->type == SDL_MOUSEBUTTONDOWN && mouse_state == MOUSE_NOTHING) {
		switch(event->button.button) {
		case SDL_BUTTON_LEFT: 
			gfx_pixel_to_world((vec2){ event->button.x, event->button.y }, begin_offset);
			mouse_state = MOUSE_DRAWING; 
			break;
		case SDL_BUTTON_RIGHT: 
			move_offset[0] = event->button.x; 
			move_offset[1] = event->button.y; 
			vec2_dup(begin_offset, offset);
			mouse_state = MOUSE_MOVING; 
			break;
		case SDL_BUTTON_MIDDLE:
			gfx_pixel_to_world((vec2){ event->button.x, event->button.y }, pos);
			
			for(CollisionData *c = editor.map->collision, *bef = NULL; c; bef = c, c = c->next) {
				vec2 min, max;

				vec2_add(max, c->position, c->half_size);
				vec2_sub(min, c->position, c->half_size);
				
				if(pos[0] < min[0] || pos[0] > max[0] || pos[1] < min[1] || pos[1] > max[1])
					continue;

				if(bef) 
					bef->next = c->next;
				if(editor.map->collision == c)
					editor.map->collision = c->next;
				
				break;
			}
			break;
		}
	}
}

void
collision_wheel(SDL_Event *event)
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
collision_render()
{
	gfx_set_camera(offset, (vec2){ zoom, zoom });

	//gfx_debug_begin();
	//for(CollisionData *c = editor.map->collision; c; c = c->next) {
	//	vec2_dup(pos, c->position);
	//	gfx_debug_quad(pos, c->half_size);
	//}

	//if(mouse_state == MOUSE_DRAWING) {
	//	gfx_debug_set_color((vec4){ 1.0, 1.0, 1.0, 1.0 });
	//	gfx_debug_quad(current_collision.position, current_collision.half_size);
	//}
	//gfx_debug_end();

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

	for(CollisionData *c = editor.map->collision; c; c = c->next) {
		gfx_draw_rect(TEXTURE_UI, c->position, c->half_size, 0.15, (vec4){ 1.0, 1.0, 1.0, 1.0 }, (vec4){ 0.0, 0.0, 1000, 1000 });
	}

	if(mouse_state == MOUSE_DRAWING) {
		gfx_draw_rect(TEXTURE_UI, current_collision.position, current_collision.half_size, 0.15, (vec4){ 1.0, 1.0, 1.0, 1.0 }, (vec4){ 0.0, 0.0, 1000, 1000 });
	}
	gfx_draw_end();
}
