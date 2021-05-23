TARGET=webapi-user

DEBUG ?= 1
OPTIMIZE ?= -O2

CC=gcc -std=gnu99 -D_DEFAULT_SOURCE -D_GNU_SOURCE
LINKER=$(CC)

CFLAGS = -Wall -Iinclude -Isrc
LIBS = -lm -lpthread -ldb -ljson-c

CFLAGS += $(shell pkg-config --cflags libsoup-2.4 libjwt)
LIBS += $(shell pkg-config --libs libsoup-2.4 libjwt)

ifeq ($(DEBUG),1)
CFLAGS += -g -D_DEBUG
OPTIMIZE = -O0
endif

SRC_DIR = src
OBJ_DIR = obj

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: do_init $(TARGET)
$(TARGET): $(OBJECTS)
	$(LINKER) $(OPTIMIZE) -o $@ $^ $(CFLAGS) $(LIBS)
	
$(OBJECTS): $(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -o $@ -c $< $(CFLAGS)
	
.PHONY: do_init clean
do_init:
	mkdir -p db obj obj/utils
	
clean:
	rm -f obj/*.o obj/utils/*.o $(TARGET)

