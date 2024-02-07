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

static void slider_cbk(UIObject obj, void *userdata)
{
	(void)userdata;
	(void)obj;

	printf("Value: %f\n", ui_slider_get_value(obj));
}

void
GAME_STATE_LEVEL_init(void)
{
	UIObject button_label = ui_label_new();
	ui_label_set_alignment(button_label, UI_LABEL_ALIGN_LEFT);
	ui_label_set_text(button_label, "Click Me!");
	
	UIObject button = ui_button_new();
	ui_button_set_callback(button, NULL, btn_callback);
	ui_button_set_label(button, button_label);

	UIObject slider = ui_slider_new();
	ui_slider_set_max_value(slider, 10);
	ui_slider_set_value(slider, 5);
	ui_slider_set_callback(slider, NULL, slider_cbk);

	UIObject label = ui_label_new();
	ui_label_set_text(label, "World");
	ui_label_set_alignment(label, UI_LABEL_ALIGN_RIGHT);

	UIObject stuff_layout = ui_layout_new();
	ui_layout_set_order(stuff_layout, UI_LAYOUT_VERTICAL);
	ui_layout_set_border(stuff_layout, 0., 0., 0., 0.);
	ui_layout_append(stuff_layout, label);
	ui_layout_append(stuff_layout, button);
	ui_layout_append(stuff_layout, slider);

	UIObject text_input = ui_text_input_new();

	UIObject layout = ui_layout_new();
	ui_layout_set_order(layout, UI_LAYOUT_VERTICAL);
	ui_layout_set_border(layout, 0., 0., 0., 0.);
	ui_layout_append(layout, stuff_layout);
	ui_layout_append(layout, text_input);

	window = ui_window_new();
	ui_window_set_size(window, (vec2){ 100, 150 });	
	ui_window_set_position(window, (vec2){ 100 + 570, 150 + 60 });
	ui_window_set_title(window, "Hello");
	ui_window_set_border(window, (vec2){ 2.0, 2.0 });
	ui_window_set_decorated(window, false);
	ui_window_set_child(window,  layout);

	ui_child_append(ui_root(), window);

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
