CFLAGS = -Wall -Wextra -Wpedantic -g -fsanitize=address -fno-omit-frame-pointer

SRC := $(shell find src -name '*.c')
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))

TARGET = server

all: $(TARGET)

$(TARGET): $(OBJ)
	gcc $(OBJ) -o $@ $(CFLAGS) $(LIBS)

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	gcc -c $< -o $@ $(CFLAGS) $(LIBS)

clean:
	rm -rf build $(TARGET)