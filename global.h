#ifndef GLOBAL_H
#define GLOBAL_H

typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Window SDL_Window;
typedef void *SDL_GLContext;

typedef struct {
	SDL_Renderer *renderer;
	SDL_Window *window;
	SDL_GLContext glctx;
} Global;

extern Global GLOBAL;

#endif
