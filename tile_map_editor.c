#include <stdio.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "global.h"
#include "vecmath.h"
#include "physics.h"
#include "entity.h"
#include "graphics.h"
#include "util.h"
#include "map.h"

Global GLOBAL;

typedef enum {
	MOUSE_NOTHING,
	MOUSE_DRAWING,
	MOUSE_MOVING,
	LAST_MOUSE_STATE
} MouseState;

typedef enum {
	EDITOR_EDIT_MAP,
	EDITOR_SELECT_TILE
} EditorState;

static void process_keydown(SDL_Event *event);
static void process_keyup(SDL_Event *event);
static void process_mouse_motion(SDL_Event *event);
static void process_mouse_press(SDL_Event *event);
static void draw_atlas();
static void draw_tilemap();
static void export_map(const char *map_file);
static void load_map(const char *map_file);

static SpriteType  map_atlas = SPRITE_TERRAIN;
static Map *map;

static vec2 offset = {0}, move_offset = {0}, begin_offset = {0};
static float zoom = 16;

static vec2 edit_offset = {0}, edit_move_offset = {0}, edit_begin_offset = {0};
static float edit_zoom = 16;

static int current_tile = 1;
static int current_layer = 0;
static bool ctrl_pressed = false;

static MouseState  mouse_state, edit_mouse_state;
static EditorState editor_state;

int
main(int argc, char *argv[])
{
	if(argc == 3) {
		map = map_alloc(atoi(argv[1]), atoi(argv[2]));
	}

	if(argc == 2) {
		load_map(argv[1]);
	}

	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		return -1;

	GLOBAL.window = SDL_CreateWindow("hello",
							  SDL_WINDOWPOS_UNDEFINED,
							  SDL_WINDOWPOS_UNDEFINED,
							  800, 600,
							  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	GLOBAL.renderer = SDL_CreateRenderer(GLOBAL.window, -1, SDL_RENDERER_ACCELERATED);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	GLOBAL.glctx = SDL_GL_CreateContext(GLOBAL.window);
	SDL_GL_MakeCurrent(GLOBAL.window, GLOBAL.glctx);

	if(glewInit() != GLEW_OK)
		return -1;

	gfx_init();

	while(true) {
		int w, h;
		SDL_Event event;
		SDL_GetWindowSize(GLOBAL.window, &w, &h);

		gfx_make_framebuffers(w, h);
		gfx_clear_framebuffers();
		gfx_setup_draw_framebuffers();
		
		switch(editor_state) {
		case EDITOR_EDIT_MAP: draw_tilemap(); break;
		case EDITOR_SELECT_TILE: draw_atlas(); break;
			break;
		}
		
		gfx_debug_end();
		
		gfx_end_draw_framebuffers();
		gfx_render_present();
		
		SDL_GL_SwapWindow(GLOBAL.window);
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				goto end_loop;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
				process_mouse_press(&event);
				break;
			case SDL_MOUSEWHEEL:
				if(editor_state == EDITOR_EDIT_MAP) {
					if(ctrl_pressed) {
						zoom += event.wheel.y;
						if(zoom < 1.0)
							zoom = 1.0;
					} else {
						current_layer += event.wheel.y;
						if(current_layer < 0)
							current_layer = 0;
						if(current_layer > SCENE_LAYERS)
							current_layer = SCENE_LAYERS;

						printf("Current Layer: %d\n", current_layer);
					}
				} else {
					edit_zoom += event.wheel.y;
					if(edit_zoom < 1.0)
						edit_zoom = 1.0;
				}
				break;
			case SDL_MOUSEMOTION:
				process_mouse_motion(&event);
				break;
			case SDL_KEYDOWN:
				process_keydown(&event);
				break;
			case SDL_KEYUP:
				process_keyup(&event);
			}
		}
	}

end_loop:
	gfx_end();

	SDL_DestroyRenderer(GLOBAL.renderer);
	SDL_DestroyWindow(GLOBAL.window);
	SDL_Quit();
	return 0;
}

void
process_mouse_press(SDL_Event *event)
{
	int x, y;
	if(editor_state == EDITOR_SELECT_TILE) {
		if(event->type == SDL_MOUSEBUTTONUP) {
			edit_mouse_state = MOUSE_NOTHING;
		}

		if(event->type == SDL_MOUSEBUTTONDOWN) {
			switch(event->button.button) {
			case SDL_BUTTON_LEFT:
				x = roundf((event->motion.x) / edit_zoom - edit_offset[0]);
				y = roundf((event->motion.y) / edit_zoom - edit_offset[1]);
				if(x < 0 || x >= map->w || y < 0 || y >= map->h) return;
				current_tile = x + y * 16;
				printf("Tile Selected: %d\n", current_tile);
				break;
			case SDL_BUTTON_RIGHT: 
				edit_move_offset[0] = event->button.x / zoom; 
				edit_move_offset[1] = event->button.y / zoom; 
				vec2_dup(edit_begin_offset, edit_offset);
				edit_mouse_state = MOUSE_MOVING; 
				break;
			}
		}
	}

	if(editor_state == EDITOR_EDIT_MAP) {
		if(event->type == SDL_MOUSEBUTTONUP) {
			mouse_state = MOUSE_NOTHING;
		}
		
		if(event->type == SDL_MOUSEBUTTONDOWN && mouse_state == MOUSE_NOTHING) {
			switch(event->button.button) {
			case SDL_BUTTON_LEFT: mouse_state = MOUSE_DRAWING; break;
			case SDL_BUTTON_RIGHT: 
				move_offset[0] = event->button.x / zoom; 
				move_offset[1] = event->button.y / zoom; 
				vec2_dup(begin_offset, offset);
				mouse_state = MOUSE_MOVING; 
				break;
			}
		}
	}
}

void
process_mouse_motion(SDL_Event *event)
{
	int x, y;

	if(editor_state == EDITOR_SELECT_TILE) {
		switch(edit_mouse_state) {
		case MOUSE_MOVING:
			edit_offset[0] = edit_begin_offset[0] + event->motion.x / zoom - edit_move_offset[0];
			edit_offset[1] = edit_begin_offset[1] + event->motion.y / zoom - edit_move_offset[1];
			break;
		case MOUSE_DRAWING:
			x = roundf((event->motion.x) / zoom - edit_offset[0]);
			y = roundf((event->motion.y) / zoom - edit_offset[1]);
			if(x < 0 || x >= 16 || y < 0 || y >= 16) return;
			current_tile = x + y * 16;
		default:
			do {} while(0);
		}
	}

	if(editor_state == EDITOR_EDIT_MAP) {
		switch(mouse_state) {
		case MOUSE_MOVING:
			offset[0] = begin_offset[0] + event->motion.x / zoom - move_offset[0];
			offset[1] = begin_offset[1] + event->motion.y / zoom - move_offset[1];
			break;
		case MOUSE_DRAWING:
			x = roundf((event->motion.x) / zoom - offset[0]);
			y = roundf((event->motion.y) / zoom - offset[1]);
			if(x < 0 || x >= map->w || y < 0 || y >= map->h) return;
			map->tiles[x + y * map->w + current_layer * map->w * map->h] = current_tile;
		default:
			do {} while(0);
		}
	}

}

void
process_keydown(SDL_Event *event)
{
	switch(event->key.keysym.sym) {
	case SDLK_1: editor_state = EDITOR_EDIT_MAP; break;
	case SDLK_2: editor_state = EDITOR_SELECT_TILE; break;
	case SDLK_LCTRL: ctrl_pressed = true; break;
	case SDLK_s:
		if(ctrl_pressed) {
			export_map("exported_map.map");
			printf("Map exported to exported_map.map\n");
		}
		break;
	}
}

void
process_keyup(SDL_Event *event) 
{
	switch(event->key.keysym.sym) {
	case SDLK_LCTRL: ctrl_pressed = false; break;
	}
}

void
draw_tilemap() 
{
	gfx_debug_begin();
	gfx_debug_set_color((vec4){ 1.0, 1.0, 1.0, 1.0 });
	for(int x = 0; x < map->w + 1; x++) {
		gfx_debug_line(
			(vec2){ (x + offset[0] - 0.5) * zoom, (0.0 + offset[1] - 0.5)        * zoom }, 
			(vec2){ (x + offset[0] - 0.5) * zoom, (map->h + offset[1] - 0.5) * zoom }
		);
	}
	for(int y = 0; y < map->h + 1; y++) {
		gfx_debug_line(
			(vec2){ (0.0       + offset[0] - 0.5) * zoom, (y + offset[1] - 0.5) * zoom }, 
			(vec2){ (map->w + offset[0] - 0.5) * zoom, (y + offset[1] - 0.5) * zoom }
		);
	}
	gfx_debug_end();
	
	gfx_draw_begin(NULL);
	for(int k = 0; k < SCENE_LAYERS; k++)
		for(int i = 0; i < map->w * map->h; i++) {
			float x = (offset[0] +      (i % map->w)) * zoom;
			float y = (offset[1] + (int)(i / map->w)) * zoom;

			if(map->tiles[i + k * map->w * map->h] <= 0)
				continue;

			if(x < 0 - zoom / 2.0 || x >= 800 + zoom / 2.0 || y < 0 - zoom / 2.0 || y > 600 + zoom / 2.0)
				continue;

			int spr = map->tiles[i + k * map->w * map->h] - 1;
			int spr_x = spr % 16;
			int spr_y = spr / 16;

			gfx_draw_sprite(&(Sprite){
					.position = { x, y },
					.sprite_id = { spr_x, spr_y },
					.color = { 1.0, 1.0, 1.0, 1.0 },
					.sprite_type = map_atlas,
					.half_size = { zoom / 2.0, zoom / 2.0 }
					});
		}
	gfx_draw_end();
}

void
draw_atlas() 
{
	gfx_debug_begin();
	gfx_debug_set_color((vec4){ 1.0, 1.0, 1.0, 1.0 });
	for(int x = 0; x < 16 + 1; x++) {
		gfx_debug_line(
			(vec2){ (x + edit_offset[0] - 0.5) * edit_zoom, (0.0 + edit_offset[1] - 0.5) * edit_zoom }, 
			(vec2){ (x + edit_offset[0] - 0.5) * edit_zoom, (16 + edit_offset[1] - 0.5)  * edit_zoom }
		);
	}
	for(int y = 0; y < 16 + 1; y++) {
		gfx_debug_line(
			(vec2){ (0.0 + edit_offset[0] - 0.5) * edit_zoom, (y + edit_offset[1] - 0.5) * edit_zoom }, 
			(vec2){ (16  + edit_offset[0] - 0.5) * edit_zoom, (y + edit_offset[1] - 0.5) * edit_zoom }
		);
	}
	gfx_debug_end();

	gfx_draw_begin(NULL);
	for(int i = 1; i < 16 * 16; i++) {
		float x = (edit_offset[0] +      (i % map->w)) * edit_zoom;
		float y = (edit_offset[1] + (int)(i / map->w)) * edit_zoom;

		if(x < 0 - edit_zoom / 2.0 || x >= 800 + edit_zoom / 2.0 || y < 0 - edit_zoom / 2.0 || y > 600 + edit_zoom / 2.0)
			continue;
		
		int spr = i - 1;
		int spr_x = spr % 16;
		int spr_y = spr / 16;

		gfx_draw_sprite(&(Sprite){
			.position = { x, y },
			.sprite_id = { spr_x, spr_y },
			.color = { 1.0, 1.0, 1.0, 1.0 },
			.sprite_type = map_atlas,
			.half_size = { edit_zoom / 2.0, edit_zoom / 2.0 }
		});
	}
	
	gfx_draw_end();
}

static void
export_map(const char *map_file) 
{
	size_t map_data_size;
	char *map_data = map_export(map, &map_data_size);
	FILE *fp = fopen(map_file, "w");
	if(!fp)
		goto error_open;
	
	int f = fwrite(map_data, 1, map_data_size, fp);
	if(f < (int)(map_data_size))
		printf("Error saving file: %s\n", map_file);
	else
		printf("File saved at %s\n", map_file);
	
	fclose(fp);
error_open:
	free(map_data);
}

void
load_map(const char *map_file)
{
	map = map_load(map_file);
	if(!map)
		die("failed loading file %s\n", map_file);
}
