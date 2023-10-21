OBJECT=main.o\
	   physics.o\
	   util.o\
	   entity.o\
	   graphics.o\
	   glutil.o\
	   impl.o\
	   graphics_scene.o\
	   entities/player.o

OUTPUT=a.out

CFLAGS=-O3 -pipe -Wall -Wextra -pedantic -Werror -g
LFLAGS=-lSDL2 -lGLEW -lGL -lm

all: $(OUTPUT)
clean:
	rm -f $(OUTPUT)
	rm -f $(OBJECT)

$(OUTPUT): $(OBJECT)
	gcc $^ -o $@ $(LFLAGS)

%.o: %.c
	gcc $< -c -o $@ $(CFLAGS)
