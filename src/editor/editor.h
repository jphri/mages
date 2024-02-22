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
	EDITOR_EDIT_COLLISION,
	LAST_EDITOR_STATE
} EditorState;

typedef struct {
	void (*init)(void);
	void (*terminate)(void);
	void (*render)(void);
	void (*wheel)(SDL_Event *event);
	void (*mouse_button)(SDL_Event *event, vec2 v_out);
	void (*mouse_motion)(SDL_Event *event, vec2 v_out);
	void (*keyboard)(SDL_Event *event);
	void (*enter)(void);
	void (*exit)(void);
} State;

typedef struct {
	Map *map;
	EditorState editor_state;
	int current_tile;
	SpriteType map_atlas;

	UIObject controls_ui;
	UIObject context_window;
	UIObject general_window;
} EditorGlobal;

int export_map(const char *map_file);
void load_map(const char *map_file);

void edit_render(void);
void edit_keyboard(SDL_Event *event);
void edit_mouse_motion(SDL_Event *event, vec2 v_out);
void edit_mouse_button(SDL_Event *event, vec2 v_out);
void edit_keyboard(SDL_Event *event);
void edit_wheel(SDL_Event *event);
void edit_enter(void);
void edit_exit(void);
void edit_init(void);
void edit_terminate(void);

void collision_render(void);
void collision_keyboard(SDL_Event *event);
void collision_mouse_motion(SDL_Event *event, vec2 v_out);
void collision_mouse_button(SDL_Event *event, vec2 v_out);
void collision_keyboard(SDL_Event *event);
void collision_wheel(SDL_Event *event);
void collision_init(void);
void collision_terminate(void);
void collision_enter(void);
void collision_exit(void);

extern EditorGlobal editor;

void editor_change_state(EditorState state);

#endif
