CC		?= gcc
TARGET	?= ./matrix
SOURCE 	:= matrix.c
CFLAGS	:= -Wall -Wextra -Wshadow -Wconversion -fsanitize=leak -fsanitize=address -fsanitize=undefined

.PHONY: all
all: build run

.PHONY: run
run: build
	$(TARGET)

.PHONY: build
build: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	$(RM) $(TARGET)
