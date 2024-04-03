#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <SDL.h>

typedef struct {
	void (*init)(void);
	void (*end)(void);
	void (*render)(int w, int h);
	void (*update)(float delta); 
	void (*keyboard)(SDL_Event *event);
	void (*mouse_move)(SDL_Event *event);
	void (*mouse_button)(SDL_Event *event);
	void (*mouse_wheel)(SDL_Event *event);
	void (*text_input)(SDL_Event *event);
} GameStateVTable;

void start_game_level(void);
void start_game_level_edit(void);

void game_change_state_vtable(GameStateVTable *new_vtable);

#endif
