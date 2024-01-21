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
static SceneTextID text;
static UIObject window;
static float time = 0;

static void pre_solve(Contact *contact);
static void btn_callback(UIObject obj, void *userdata)
{
	(void)userdata;
	(void)obj;

	printf("Hello, world!\n");
}

void
GAME_STATE_LEVEL_init(void)
{
	for(int i = 0; i < 2; i++) {
		window = ui_new_object(ui_root(), UI_FLOAT);
		UI_FLOAT_struct *wdata = ui_data(window);

		//wdata->title = "Hello";
		//wdata->border[0] = 5.0;
		//wdata->border[1] = 5.0;
		//vec4_dup(wdata->background,   (vec4){ 0.4, 0.4, 0.4, 1.0 });
		//vec4_dup(wdata->border_color, (vec4){ 0.2, 0.2, 0.2, 1.0 });
		vec2_dup(wdata->rect.position, (vec2){ 100 + 570 - 90 * i, 75 + 60 });
		vec2_dup(wdata->rect.half_size, (vec2){ 100, 75 });

		UIObject layout = ui_new_object(window, UI_LAYOUT);
		UI_LAYOUT_struct *laydata = ui_data(layout);
		laydata->order = UI_LAYOUT_VERTICAL;

		UIObject label = ui_new_object(layout, UI_LABEL);
		UI_LABEL_struct *ldata = ui_data(label);
		ldata->label_ptr = "World";

		UIObject button = ui_new_object(layout, UI_BUTTON);
		UI_BUTTON_struct *bdata = ui_data(button);
		bdata->label = "Click Me.";
		bdata->callback = btn_callback;

		UIObject slider = ui_new_object(layout, UI_SLIDER);
		UI_SLIDER_struct *sdata = ui_data(slider);
		sdata->max_value = 10;
		sdata->value = 5;
	}

	map = editor.map;
	map_set_gfx_scene(map);
	map_set_phx_scene(map);
	
	GLOBAL.player_id = ent_player_new((vec2){ 15.0, 15.0 });
	ent_dummy_new((vec2){ 25, 15 });
	
	text = gfx_scene_new_obj(0, SCENE_OBJECT_TEXT);
	vec4_dup(gfx_scene_text(text)->color, (vec4){ 1.0, 1.0, 1.0, 1.0 });
	vec2_dup(gfx_scene_text(text)->position, (vec2){ 0.0, 0.0 });
	vec2_dup(gfx_scene_text(text)->char_size, (vec2){ 0.05, 0.05 });
	gfx_scene_text(text)->text_ptr = (RelPtr){ .base_pointer = (void**)&text_test, .offset = 0 };

	phx_set_pre_solve(pre_solve);
}

void
GAME_STATE_LEVEL_update(float delta)
{
	time += delta;
	const float f = fabs(sinf(time)) * 0.25;

	vec2_dup(gfx_scene_text(text)->char_size, (vec2){ f, f });
	
	if(time > 2.0 && window != 0) {
		ui_del_object(window);
		window = 0;
	}
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
