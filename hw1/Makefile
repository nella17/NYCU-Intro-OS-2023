CC		?= gcc
TARGET	?= ./shell
SOURCE 	:= shell.c
CFLAGS	:= -Wall -Wextra -Wshadow -Wconversion

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
