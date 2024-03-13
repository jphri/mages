#include <stdio.h>
#include <stdbool.h>
#include <glad/gles2.h>

#include <SDL.h>

#include "../graphics.h"
#include "../map.h"
#include "../editor/editor.h"
#include "../game_state.h"
#include "../global.h"
#include "../vecmath.h"
#include "../physics.h"
#include "../entity.h"
#include "../map.h"
#include "../ui.h"
#include "../util.h"
#include "../audio.h"
#include "../events.h"
#include "SDL_events.h"


static void event_receiver(Event event, const void *data);
static void edit_cbk(UIObject *obj, void *userptr);

static Map *map;

static Subscriber *level_subscriber;
static vec2 camera_position;
static vec2 mouse_pos;

static Door *door_test;

void
GAME_STATE_LEVEL_init(void)
{
	GLOBAL.player = NULL;

	UIObject *file_buttons_window = ui_window_new();
	ui_window_set_decorated(file_buttons_window, false);
	ui_window_set_size(file_buttons_window, (vec2){ 40, 40 });
	ui_window_set_position(file_buttons_window, UI_ORIGIN_TOP_RIGHT, (vec2){ -40, 40 });
	{
		UIObject *layout = ui_layout_new();
		ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
		{
			UIObject *btn;

			#define CREATE_BUTTON(name, userptr, cbk) \
			btn = ui_button_new(); \
			ui_button_set_callback(btn, userptr, cbk); \
			{\
				UIObject *lbl = ui_label_new(); \
				ui_label_set_text(lbl, name); \
				ui_label_set_color(lbl, (vec4){ 1.0, 1.0, 0.0, 1.0 }); \
				ui_button_set_label(btn, lbl); \
			} \
			ui_layout_append(layout, btn)
			
			CREATE_BUTTON("Edit", NULL, edit_cbk);
		}

		ui_window_append_child(file_buttons_window, layout);
	}
	ui_child_append(ui_root(), file_buttons_window);

	level_subscriber = event_create_subscriber(event_receiver);
	event_subscribe(level_subscriber, EVENT_PLAYER_SPAWN);

	map = editor.map;
	map_set_gfx_scene(map);
	map_set_phx_scene(map);
	map_set_ent_scene(map);

	door_test = ent_door_new((vec2){ 0.0, 0.0 }, DIR_LEFT);
	door_test->open = 1;
}

void
GAME_STATE_LEVEL_update(float delta)
{
	vec2 offset, delta_pos;
	float dist2;
	Rectangle window_rect = gfx_window_rectangle();

	if(GLOBAL.player) {
		vec2_add_scaled(offset, (vec2){ 0.0, 0.0 }, GLOBAL.player->player.body->position, -32.0);
		vec2_add(offset, offset, window_rect.position);

		vec2_sub(delta_pos, offset, camera_position);
		dist2 = sqrtf(delta_pos[0] * delta_pos[0] + delta_pos[1] * delta_pos[1]) * 2;
		vec2_normalize(delta_pos, delta_pos);

		vec2_add_scaled(camera_position, camera_position, delta_pos, dist2 * delta);
		gfx_set_camera(camera_position, (vec2){ 32.0, 32.0 });
	}
}

void
GAME_STATE_LEVEL_render(void)
{
}

void
GAME_STATE_LEVEL_end(void)
{
	phx_reset();
	ent_reset();
	ui_reset();
	gfx_scene_reset();
	audio_bgm_pause();

	event_delete_subscriber(level_subscriber);
}

void 
GAME_STATE_LEVEL_mouse_move(SDL_Event *event) 
{ 
	(void)event;
}

void 
GAME_STATE_LEVEL_mouse_button(SDL_Event *event)
{ 
	if(event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_RIGHT) {
		gfx_pixel_to_world((vec2){ event->button.x, event->button.y }, mouse_pos);
		ent_mouse_interact(&GLOBAL.player->player, mouse_pos);
	}
}

void 
GAME_STATE_LEVEL_keyboard(SDL_Event *event) 
{ 
	(void)event;
}

void GAME_STATE_LEVEL_mouse_wheel(SDL_Event *event) { (void)event; } 

void 
edit_cbk(UIObject *obj, void *userptr)
{
	(void)obj;
	(void)userptr;
	gstate_set(GAME_STATE_LEVEL_EDIT);
}

void
event_receiver(Event event, const void *data)
{
	const EVENT(EVENT_PLAYER_SPAWN) ev;
	
	switch(event) {
	case EVENT_PLAYER_SPAWN:
		ev = data;
		GLOBAL.player = ev->player;
	default:
		break;
	}
}
