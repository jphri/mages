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
CFLAGS += -I$(SDL2_INCLUDE_DIR)
LFLAGS=-L$(SDL2_LIB_DIR) -lSDL2 -lSDL2main -lm

DELETE = rm -f

ifeq ($(OS),Windows_NT)
	DELETE := del /f
	
	OBJ_FILES        := $(subst /,\,$(OBJ_FILES))
	GAME_OBJ_FILES   := $(subst /,\,$(GAME_OBJ_FILES))
	EDITOR_OBJ_FILES := $(subst /,\,$(EDITOR_OBJ_FILES))
	DEPFILES         := $(subst /,\,$(DEPFILES))
	THIRD_OBJ_FILES  := $(subst /,\,$(THIRD_OBJ_FILES))
	
	THIRD_SRC_FILES     := $(subst /,\,$(THIRD_SRC_FILES))
	EDITOR_SOURCE_FILES := $(subst /,\,$(EDITOR_SOURCE_FILES))
	GAME_SOURCE_FILES   := $(subst /,\,$(GAME_SOURCE_FILES))
	SRC_FILES           := $(subst /,\,$(SRC_FILES))
	
	DEPFILES           := $(subst /,\,$(DEPFILES)) 
endif

all: $(OUTPUT) $(EDITOR)

clean:
	$(DELETE) $(OBJ_FILES)
	$(DELETE) $(GAME_OBJ_FILES)
	$(DELETE) $(EDITOR_OBJ_FILES)
	$(DELETE) $(OUTPUT) $(EDITOR)
	$(DELETE) $(DEPFILES)

nuke: clean
	$(DELETE) $(THIRD_OBJ_FILES)

$(OUTPUT): $(THIRD_OBJ_FILES) $(OBJ_FILES) $(GAME_OBJ_FILES)
	$(CC) $^ -o $@ $(LFLAGS)

$(EDITOR): $(THIRD_OBJ_FILES) $(OBJ_FILES) $(EDITOR_OBJ_FILES)
	$(CC) $^ -o $@ $(LFLAGS)

%.o: %.c
	$(CC) $< -c -o $@ $(CFLAGS)

-include $(DEPFILES)
