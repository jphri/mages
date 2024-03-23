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

	UIObject *controls_ui;
	UIObject *thing_window, *brush_window;
	UIObject *general_window;
} EditorGlobal;

int export_map(const char *map_file);
void load_map(const char *map_file);

extern EditorGlobal editor;

void editor_move_camera(vec2 delta);
void editor_delta_zoom(float delta);
float editor_get_zoom(void);

#endif
