#ifndef GLOBAL_H
#define GLOBAL_H

#include <SDL2/SDL.h>

typedef struct {
	SDL_Renderer *renderer;
	SDL_Window *window;
	SDL_GLContext glctx;
} Global;

extern Global GLOBAL;

#endif
