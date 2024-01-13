#These 4 variables can be set on the command line
SDL2_PREFIX  = /usr
CC           = cc

THIRD_SRC_FILES = $(wildcard third/src/*.c)
SRC_FILES       = $(wildcard src/*.c src/entities/*.c src/game/*.c src/editor/*.c) 

OBJ_FILES       = $(SRC_FILES:.c=.o)
THIRD_OBJ_FILES = $(THIRD_SRC_FILES:.c=.o)

DEPFILES  = $(OBJ_FILES:.o=.d)
DEPFILES += $(THIRD_OBJ_FILES:.o=.d)

SDL2_INCLUDE = -I$(SDL2_PREFIX)/include/SDL2
SDL2_LIB     = -L$(SDL2_PREFIX)/lib -lSDL2 -lSDL2main

OUTPUT       = game

CFLAGS += -DSDL_MAIN_HANDLED
CFLAGS += -O3 -std=c99 -pipe -Wall -Wextra -Werror -pedantic
CFLAGS += -Ithird/include
CFLAGS += $(SDL2_INCLUDE)
CFLAGS += -fPIE
CFLAGS += -MP -MD
LFLAGS += $(SDL2_LIB)
LFLAGS += -lm
LFLAGS += -fPIE

ifeq ($(DEBUG),yes)
	CFLAGS += -g
endif

ifeq ($(SANITIZE),yes)
	CFLAGS += -fsanitize=address
	CFLAGS += -fsanitize=bounds
	CFLAGS += -fsanitize=undefined
	LFLAGS += -fsanitize=address
	LFLAGS += -fsanitize=bounds
	LFLAGS += -fsanitize=undefined
endif

DELETE = rm -f

ifeq ($(OS),Windows_NT)
	DELETE := rm -f
	
	THIRD_SRC_FILES     := $(subst /,\,$(THIRD_SRC_FILES))
	SRC_FILES           := $(subst /,\,$(SRC_FILES))

	OBJ_FILES           := $(subst /,\,$(OBJ_FILES))
	DEPFILES            := $(subst /,\,$(DEPFILES))
	THIRD_OBJ_FILES     := $(subst /,\,$(THIRD_OBJ_FILES))
	
	DEPFILES            := $(subst /,\,$(DEPFILES)) 

	LFLAGS              += -lmingw32 -mwindows 
	OUTPUT              := $(OUTPUT).exe
endif

.PHONY: info clean nuke all

all: info $(OUTPUT)

info:
	@echo SDL2_INCLUDE = $(SDL2_INCLUDE)
	@echo SDL2_LIB = $(SDL2_LIB)
	@echo OUTPUT = $(OUTPUT)
	@echo CC = $(CC)

clean:
	$(DELETE) $(OBJ_FILES)
	$(DELETE) $(OUTPUT)
	$(DELETE) $(DEPFILES)

nuke: clean
	$(DELETE) $(THIRD_OBJ_FILES)

$(OUTPUT): $(THIRD_OBJ_FILES) $(OBJ_FILES) $(GAME_OBJ_FILES) $(EDITOR_OBJ_FILES)
	$(CC) $^ $(LFLAGS) -o $@

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@

-include $(DEPFILES)
