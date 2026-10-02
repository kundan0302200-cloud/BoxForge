CC := gcc
CFLAGS := -std=c11 -D_GNU_SOURCE -Wall -Wextra -Wpedantic -O2 -g -Iinclude
LDFLAGS :=

TARGET := boxforge
SRC := src/main.c src/container.c src/namespaces.c src/filesystem.c src/cgroup.c src/lifecycle.c src/utils.c
OBJ := $(SRC:src/%.c=build/%.o)

.PHONY: all clean debug

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

debug: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer

debug: clean all

clean:
	rm -rf build $(TARGET)
