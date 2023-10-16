typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Window SDL_Window;

typedef struct {
	SDL_Renderer *renderer;
	SDL_Window *window;
} Global;

extern Global GLOBAL;
