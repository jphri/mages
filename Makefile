OBJECT=main.o\
	   physics.o\
	   util.o\
	   entity.o\
	   entities/player.o

OUTPUT=a.out

CFLAGS=-O3 -pipe
LFLAGS=-lSDL2

all: $(OUTPUT)
clean:
	rm -f $(OUTPUT)
	rm -f $(OBJECT)

$(OUTPUT): $(OBJECT)
	gcc $^ -o $@ $(LFLAGS)

%.o: %.c
	gcc $< -c -o $@ $(CFLAGS)
