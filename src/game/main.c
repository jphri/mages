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

static void event_receiver(Event event, const void *data);

static Map *map;

static void pre_solve(Contact *contact);
static void edit_cbk(UIObject obj, void *userptr);

static SubscriberID level_subscriber;

void
GAME_STATE_LEVEL_init(void)
{
	UIObject file_buttons_window = ui_window_new();
	ui_window_set_decorated(file_buttons_window, false);
	ui_window_set_size(file_buttons_window, (vec2){ 40, 40 });
	ui_window_set_position(file_buttons_window, UI_ORIGIN_TOP_RIGHT, (vec2){ -40, 40 });
	{
		UIObject layout = ui_layout_new();
		ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
		{
			UIObject btn;

			#define CREATE_BUTTON(name, userptr, cbk) \
			btn = ui_button_new(); \
			ui_button_set_callback(btn, userptr, cbk); \
			{\
				UIObject lbl = ui_label_new(); \
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

	phx_set_pre_solve(pre_solve);
}

void
GAME_STATE_LEVEL_update(float delta)
{
	(void)delta;
}

void
GAME_STATE_LEVEL_render(void)
{
	vec2 offset;
	#define PLAYER ENT_DATA(ENTITY_PLAYER, GLOBAL.player_id)
	#define PLAYER_BODY phx_data(PLAYER->body.body)
	if(GLOBAL.player_id) {
		vec2_add_scaled(offset, (vec2){ 0.0, 0.0 }, PLAYER_BODY->position, -32);
		vec2_add(offset, offset, (vec2){ 400, 300 });
		gfx_set_camera(offset, (vec2){ 32.0, 32.0 });
	}
	#undef PLAYER
	#undef PLAYER_BODY
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

void GAME_STATE_LEVEL_mouse_move(SDL_Event *event) { (void)event; }
void GAME_STATE_LEVEL_mouse_button(SDL_Event *event) { (void)event; }

void 
GAME_STATE_LEVEL_keyboard(SDL_Event *event) 
{ 
	(void)event;
}

void GAME_STATE_LEVEL_mouse_wheel(SDL_Event *event) { (void)event; } 

static void process_entity_collision(EntityID id, BodyID other, Contact *contact)
{
	EntityBody *ent_b = ent_component(id, ENTITY_COMP_BODY);
	if(ent_b && ent_b->pre_solve)
		ent_b->pre_solve(id, other, contact);
}

void
pre_solve(Contact *contact)
{
	Body* b1 = phx_data(contact->body1);
	Body* b2 = phx_data(contact->body2);

	GameObjectID id1 = b1->user_data;
	GameObjectID id2 = b2->user_data;

	if(gobj_type(id1) == GAME_OBJECT_TYPE_ENTITY)
		process_entity_collision(gobj_id(id1), contact->body2, contact);

	if(gobj_type(id2) == GAME_OBJECT_TYPE_ENTITY)
		process_entity_collision(gobj_id(id2), contact->body1, contact);
}

void 
edit_cbk(UIObject obj, void *userptr)
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
		GLOBAL.player_id = ev->player_id;
	default:
		break;
	}
}
