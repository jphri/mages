#include "../ui.h"
#include "../graphics.h"

#define IMAGE(OBJ) WIDGET(UI_IMAGE, OBJ)

UIObject *
ui_image_new(void)
{
	UIObject *obj = ui_new_object(0, UI_IMAGE);
	ui_image_set_stamp(obj, &(TextureStamp){0});
	return obj;
}

void
ui_image_set_stamp(UIObject *image, TextureStamp *stamp)
{
	IMAGE(image)->image_stamp = *stamp;
}

void
ui_image_set_keep_aspect(UIObject *image, bool keep)
{
	IMAGE(image)->keep_aspect = keep;
}

void
UI_IMAGE_event(UIObject *image, UIEvent *event, Rectangle *rect)
{
	if(event->event_type != UI_DRAW)
		return;

	Rectangle r;
	float aspect = 1.0;
	if(IMAGE(image)->keep_aspect) {
		aspect = IMAGE(image)->image_stamp.size[0] / IMAGE(image)->image_stamp.size[1];
	}
	vec2_dup(r.position, rect->position);
	r.half_size[1] = rect->half_size[1];
	r.half_size[0] = r.half_size[1] * aspect;

	if(r.half_size[0] > rect->half_size[0])  {
		r.half_size[0] = rect->half_size[0];
		r.half_size[1] = r.half_size[0] / aspect;
	}

	gfx_draw_texture_rect(
		&IMAGE(image)->image_stamp, 
		r.position, 
		r.half_size,
		0.0, 
		(vec4){ 1.0, 1.0, 1.0, 1.0 });
}
