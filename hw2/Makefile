CC		?= gcc
TARGET	?= ./matrix.exe
SOURCE 	:= matrix.c
CFLAGS	+= -Wall -Wextra -Wshadow -Wconversion -g
ifneq ($(shell uname),Darwin)
CFLAGS 	+= -fsanitize=leak -fsanitize=address -fsanitize=undefined
endif

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
