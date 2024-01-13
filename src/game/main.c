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

static Map *map;
//static UIObject label;

static char *text_test = "Hello";

static void
edit_button_cbk(void *ptr) 
{
	(void)ptr;
	gstate_set(GAME_STATE_LEVEL_EDIT);
}

static void pre_solve(Contact *contact);

void
GAME_STATE_LEVEL_init(void)
{
	UIObject window = ui_new_object(0, UI_WINDOW);
	ui_obj_set_size(window, (vec2){ 90, 35 });
	ui_obj_set_position(window, (vec2){ 800 - 90, 35 });
	ui_window_set_bg(window, (vec4){ 0.2, 0.2, 0.2, 1.0 });
	ui_window_set_border(window, (vec4){ 0.6, 0.6, 0.6, 1.0 });
	ui_window_set_border_size(window, (vec2){ 3, 3 });

	UIObject layout3 = ui_new_object(window, UI_LAYOUT);
	ui_obj_set_size(layout3, (vec2){ 70, 35 });
	ui_obj_set_position(layout3, (vec2) { 90, 35 });
	ui_layout_set_order(layout3, UI_LAYOUT_VERTICAL);

	//label = ui_new_object(layout3, UI_LABEL);
	//ui_label_set_color(label, (vec4){ 1.0, 1.0, 0.0, 1.0 });
	//ui_label_set_border(label, (vec2){ 10.0, 10.0 });
	//ui_label_set_text(label, "Testing");

	UIObject edit_button = ui_new_object(layout3, UI_BUTTON);
	ui_button_set_userptr(edit_button, NULL);
	ui_button_set_label(edit_button, "Edit");
	ui_button_set_label_border(edit_button, (vec2){ 2.0, 2.0 });
	ui_button_set_callback(edit_button, edit_button_cbk);

	map = editor.map;
	map_set_gfx_scene(map);
	map_set_phx_scene(map);
	
	GLOBAL.player_id = ent_player_new((vec2){ 15.0, 15.0 });
	ent_dummy_new((vec2){ 25, 15 });
	
	SceneTextID text = gfx_scene_new_obj(0, SCENE_OBJECT_TEXT);
	vec4_dup(gfx_scene_text(text)->color, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	vec2_dup(gfx_scene_text(text)->position, (vec2){ 0.0, 0.0 });
	vec2_dup(gfx_scene_text(text)->char_size, (vec2){ 0.35, 0.35 });
	gfx_scene_text(text)->text_ptr = (RelPtr){ .base_pointer = (void**)&text_test, .offset = 0 };

	phx_set_pre_solve(pre_solve);
	audio_bgm_play(AUDIO_BUFFER_BGM_TEST, 1.0);
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
	gfx_scene_reset();
	ui_reset();

	audio_bgm_pause();
}

void GAME_STATE_LEVEL_mouse_move(SDL_Event *event) { (void)event; }
void GAME_STATE_LEVEL_mouse_button(SDL_Event *event) { (void)event; }
void 
GAME_STATE_LEVEL_keyboard(SDL_Event *event) 
{ 
	if(event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_K)
		gstate_set(GAME_STATE_LEVEL_EDIT);
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
