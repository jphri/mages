#ifndef EDITOR_H
#define EDITOR_H

#include "../map.h"
#include "../graphics.h"
#include "../ui.h"
#include <SDL.h>

typedef enum {
	MOUSE_NOTHING,
	MOUSE_DRAWING,
	MOUSE_MOVING,
	LAST_MOUSE_STATE
} MouseState;

typedef enum {
	EDITOR_EDIT_MAP,
	EDITOR_SELECT_TILE,
	EDITOR_EDIT_COLLISION,
	LAST_EDITOR_STATE
} EditorState;

typedef struct {
	void (*init)(void);
	void (*terminate)(void);
	void (*render)(void);
	void (*wheel)(SDL_Event *event);
	void (*mouse_button)(SDL_Event *event);
	void (*mouse_motion)(SDL_Event *event);
	void (*keyboard)(SDL_Event *event);
	void (*enter)(void);
	void (*exit)(void);
} State;

typedef struct {
	Map *map;
	EditorState editor_state;
	int current_tile;
	SpriteType map_atlas;
} EditorGlobal;

void export_map(const char *map_file);
void load_map(const char *map_file);

void edit_render(void);
void edit_keyboard(SDL_Event *event);
void edit_mouse_motion(SDL_Event *event);
void edit_mouse_button(SDL_Event *event);
void edit_keyboard(SDL_Event *event);
void edit_wheel(SDL_Event *event);
void edit_enter(void);
void edit_exit(void);
void edit_init(void);
void edit_terminate(void);

void select_tile_render(void);
void select_tile_keyboard(SDL_Event *event);
void select_tile_mouse_motion(SDL_Event *event);
void select_tile_mouse_button(SDL_Event *event);
void select_tile_keyboard(SDL_Event *event);
void select_tile_wheel(SDL_Event *event);

void collision_render(void);
void collision_keyboard(SDL_Event *event);
void collision_mouse_motion(SDL_Event *event);
void collision_mouse_button(SDL_Event *event);
void collision_keyboard(SDL_Event *event);
void collision_wheel(SDL_Event *event);

extern EditorGlobal editor;

void editor_change_state(EditorState state);

#endif
