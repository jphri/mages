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
		entities/dummy.o\
		entities/fireball.o\
		map.o\
		ui.o\
		third/glad/src/gles2.o

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
		ui.o\
		third/glad/src/gles2.o

PLATFORM=
SDL2_PREFIX=/usr
SDL2_INCLUDE_DIR=$(SDL2_PREFIX)/$(PLATFORM)/include
SDL2_LIB_DIR=$(SDL2_PREFIX)/$(PLATFORM)/lib
OUTPUT=game
EDITOR=editor
CFLAGS=-O3 -std=c99 -pipe -Wall -Wextra -Werror -pedantic -g -Ithird/glad/include -I$(SDL2_INCLUDE_DIR)
LFLAGS=-L$(SDL2_LIB_DIR) -lSDL2 -lm

all: $(OUTPUT) $(EDITOR)
clean:
	rm -f $(OUTPUT_OBJECTS)
	rm -f $(OUTPUT)
	rm -f $(EDITOR_OBJECTS)
	rm -f $(EDITOR)

nuke: clean
	rm -f $(THIRD_OBJECTS)

$(OUTPUT): $(OUTPUT_OBJECTS) $(THIRD_OBJECTS)
	$(PLATFORM)-gcc $^ -o $@ $(LFLAGS)

$(EDITOR): $(EDITOR_OBJECTS) $(THIRD_OBJECTS)
	$(PLATFORM)-gcc $^ -o $@ $(LFLAGS)

%.o: %.c
	$(PLATFORM)-gcc $< -c -o $@ $(CFLAGS)
