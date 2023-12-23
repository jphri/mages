#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <SDL2/SDL.h>

/* the first is also the starting state */
#define GAME_STATE_LIST \
	GAME_ST_DEF(GAME_STATE_LEVEL_EDIT) \
	GAME_ST_DEF(GAME_STATE_LEVEL) \

typedef enum {
	#define GAME_ST_DEF(STATE_NAME) \
		STATE_NAME,

	GAME_STATE_LIST

	#undef GAME_ST_DEF
	LAST_GAME_STATE
} GameState;

#define GAME_ST_DEF(STATE_NAME) \
	void STATE_NAME##_init(void), \
		 STATE_NAME##_end(void),  \
	     STATE_NAME##_update(float delta), \
		 STATE_NAME##_render(void),  \
		 STATE_NAME##_keyboard(SDL_Event *event), \
		 STATE_NAME##_mouse_move(SDL_Event *event), \
		 STATE_NAME##_mouse_button(SDL_Event *event), \
		 STATE_NAME##_mouse_wheel(SDL_Event *event);

GAME_STATE_LIST

#undef GAME_ST_DEF

void gstate_set(GameState state);

/* defined in init.c */

#endif
