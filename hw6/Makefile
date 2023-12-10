CXX		?= g++
NAME 	:= dedup
TARGET	?= ./${NAME}.exe
SOURCE 	:= ${NAME}.cpp
CPPFLAGS += -Wall -Wextra -Wshadow -Wconversion -g
ifneq ($(shell uname),Darwin)
CPPFLAGS += -fsanitize=leak -fsanitize=address -fsanitize=undefined
endif

.PHONY: all
all: build run

.PHONY: run
run: build
	$(TARGET)

.PHONY: build
build: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CPPFLAGS) $^ -o $@

.PHONY: clean
clean:
	$(RM) $(TARGET)
