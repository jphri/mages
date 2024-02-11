#include "../ui.h"

#define SPRITE_SIZE 16

#define TSET(OBJ) WIDGET(UI_TILESET_SEL, OBJ)

UIObject 
ui_tileset_sel_new(void)
{
	UIObject tileset =  ui_new_object(0, UI_TILESET_SEL);
	gfx_sprite_count_rows_cols(SPRITE_TERRAIN, &TSET(tileset)->rows, &TSET(tileset)->cols);
	return tileset;
}

void
UI_TILESET_SEL_event(UIObject obj, UIEvent *ev, Rectangle *r)
{
	(void)obj;
	vec2 line_min, line_max;

	rect_boundaries(line_min, line_max, r);

	line_max[0] = line_min[0] + TSET(obj)->rows * SPRITE_SIZE;
	line_max[1] = line_min[1] + TSET(obj)->cols * SPRITE_SIZE;

	switch(ev->event_type) {
	case UI_DRAW:
		for(int i = 0; i < TSET(obj)->rows * TSET(obj)->cols; i++) {
			float x = (     (i % TSET(obj)->cols) + 0.5) * SPRITE_SIZE + line_min[0];
			float y = ((int)(i / TSET(obj)->cols) + 0.5) * SPRITE_SIZE + line_min[1];

			int spr = i - 1;
			int spr_x = spr % TSET(obj)->cols;
			int spr_y = spr / TSET(obj)->cols;

			TextureStamp sprite = {
				.texture = TERRAIN_NORMAL,
				.position = {
					spr_x / (float)TSET(obj)->cols,
					spr_y / (float)TSET(obj)->rows,
				},
				.size = {
					1.0 / (float)TSET(obj)->cols,
					1.0 / (float)TSET(obj)->rows,
				}
			};
			gfx_draw_texture_rect(&sprite, (vec2){ x, y }, (vec2){ SPRITE_SIZE / 2.0, SPRITE_SIZE / 2.0 }, 0.0, (vec4){ 1.0, 1.0, 1.0, 1.0 });
		}

		for(int x = 0; x <= TSET(obj)->cols; x++) {
			gfx_draw_line(
				(vec2){ line_min[0] + x * SPRITE_SIZE, line_min[1] }, 
				(vec2){ line_min[0] + x * SPRITE_SIZE, line_max[1] },
				1.0, 
				(vec4){ 1.0, 1.0, 1.0, 1.0 }
			);
		}

		for(int y = 0; y <= TSET(obj)->rows; y++) {
			gfx_draw_line(
				(vec2){ line_min[0], line_min[1] + y * SPRITE_SIZE }, 
				(vec2){ line_max[0], line_min[1] + y * SPRITE_SIZE },
				1.0, 
				(vec4){ 1.0, 1.0, 1.0, 1.0 }
			);
		}
		break;
	default:
		break;
	}
}

