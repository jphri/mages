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
	MOUSE_MOVING_BRUSH,
	LAST_MOUSE_STATE
} MouseState;

typedef enum {
	EDITOR_EDIT_MAP,
	EDITOR_EDIT_COLLISION,
	EDITOR_EDIT_THINGS,
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

	Thing *layers[64];

	UIObject *controls_ui;
	UIObject *context_window;
	UIObject *general_window;
} EditorGlobal;

int export_map(const char *map_file);
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

void thing_render(void);
void thing_keyboard(SDL_Event *event);
void thing_mouse_motion(SDL_Event *event);
void thing_mouse_button(SDL_Event *event);
void thing_keyboard(SDL_Event *event);
void thing_wheel(SDL_Event *event);
void thing_enter(void);
void thing_exit(void);
void thing_init(void);
void thing_terminate(void);

void collision_render(void);
void collision_keyboard(SDL_Event *event);
void collision_mouse_motion(SDL_Event *event);
void collision_mouse_button(SDL_Event *event);
void collision_keyboard(SDL_Event *event);
void collision_wheel(SDL_Event *event);
void collision_init(void);
void collision_terminate(void);
void collision_enter(void);
void collision_exit(void);

void common_draw_map(int current_layer, float alpha_after_layer);

extern EditorGlobal editor;

void editor_change_state(EditorState state);

void editor_move_camera(vec2 delta);
void editor_delta_zoom(float delta);
float editor_get_zoom(void);

#endif
