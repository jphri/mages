#ifndef GLOBAL_H
#define GLOBAL_H

#include <SDL.h>
#include "util.h"
#include "defs.h"

typedef struct {
	SDL_Renderer *renderer;
	SDL_Window *window;
	SDL_GLContext glctx;

	Entity *player;
} Global;

extern Global GLOBAL;
Allocator cache_aligned_allocator(void);
void      enable_text_input(void);
void      disable_text_input(void);

#endif
