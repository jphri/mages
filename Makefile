THIRD_SRC_FILES=$(wildcard third/src/*.c)
EDITOR_SOURCE_FILES=$(wildcard src/editor/*.c)
GAME_SOURCE_FILES=$(wildcard src/game/*.c)
SRC_FILES=$(wildcard src/*.c src/entities/*.c) 

OBJ_FILES=$(SRC_FILES:.c=.o)
EDITOR_OBJ_FILES=$(EDITOR_SOURCE_FILES:.c=.o)
GAME_OBJ_FILES=$(GAME_SOURCE_FILES:.c=.o)
THIRD_OBJ_FILES=$(THIRD_SRC_FILES:.c=.o)

DEPFILES  = $(OBJ_FILES:.o=.d)
DEPFILES += $(EDITOR_OBJ_FILES:.o=.d) 
DEPFILES += $(GAME_OBJ_FILES:.o=.d) 
DEPFILES += $(THIRD_OBJ_FILES:.o=.d)

PLATFORM=
SDL2_PREFIX=/usr
SDL2_INCLUDE_DIR=$(SDL2_PREFIX)/$(PLATFORM)/include
SDL2_LIB_DIR=$(SDL2_PREFIX)/$(PLATFORM)/lib
OUTPUT=game
EDITOR=editor
CC=$(PLATFORM)-gcc

CFLAGS += -O3 -std=c99 -pipe -Wall -Wextra -Werror -pedantic -g
CFLAGS += -MP -MD
CFLAGS += -Ithird/glad/include
CFLAGS += -Ithird

LFLAGS=-L$(SDL2_LIB_DIR) -lSDL2 -lm

all: $(OUTPUT) $(EDITOR)
clean:
	rm -f $(OBJ_FILES)
	rm -f $(GAME_OBJ_FILES)
	rm -f $(EDITOR_OBJ_FILES)
	rm -f $(OUTPUT) $(EDITOR)
	rm -f $(DEPFILES)

nuke: clean
	rm -f $(THIRD_OBJ_FILES)

$(OUTPUT): $(THIRD_OBJ_FILES) $(OBJ_FILES) $(GAME_OBJ_FILES)
	$(CC) $^ -o $@ $(LFLAGS)

$(EDITOR): $(THIRD_OBJ_FILES) $(OBJ_FILES) $(EDITOR_OBJ_FILES)
	$(CC) $^ -o $@ $(LFLAGS)

%.o: %.c
	$(CC) $< -c -o $@ $(CFLAGS)

-include $(DEPFILES)
