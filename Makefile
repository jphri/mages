THIRD_OBJECTS=impl.o

OUTPUT_OBJECTS=\
		main.o\
		physics.o\
		util.o\
		entity.o\
		graphics.o\
		glutil.o\
		graphics_scene.o\
		entities/player.o\
		map.o\
		ui.o

OUTPUT=a.out

EDITOR_OBJECTS=\
		tile_map_editor.o\
		physics.o\
		util.o\
		graphics.o\
		graphics_scene.o\
		glutil.o\
		map.o\
		editor_states/edit_tiles.o\
		editor_states/select_tiles.o\
		editor_states/edit_collisions.o\
		ui.o

EDITOR=editor

CFLAGS=-O3 -pipe -Wall -Wextra -pedantic -Werror -g
LFLAGS=-lSDL2 -lGLEW -lGL -lm

all: $(OUTPUT) $(EDITOR)
clean:
	rm -f $(OUTPUT_OBJECTS)
	rm -f $(OUTPUT)
	rm -f $(EDITOR_OBJECTS)
	rm -f $(EDITOR)

nuke: clean
	rm -f $(THIRD_OBJECTS)

$(OUTPUT): $(OUTPUT_OBJECTS) $(THIRD_OBJECTS)
	gcc $^ -o $@ $(LFLAGS)

$(EDITOR): $(EDITOR_OBJECTS) $(THIRD_OBJECTS)
	gcc $^ -o $@ $(LFLAGS)

%.o: %.c
	gcc $< -c -o $@ $(CFLAGS)
