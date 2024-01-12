#ifndef GLOBAL_H
#define GLOBAL_H

#include <SDL2/SDL.h>
#include "game_objects.h"
#include "util.h"

typedef struct {
	SDL_Renderer *renderer;
	SDL_Window *window;
	SDL_GLContext glctx;
	
	EntityID player_id;
} Global;

extern Global GLOBAL;
Allocator cache_aligned_allocator();

#endif
