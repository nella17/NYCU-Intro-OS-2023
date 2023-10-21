CC		?= gcc
PROG 	:= sort.exe
GEN 	:= sort-gen.exe
BINDIR	?= ./
SRCDIR	?= ./

CFLAGS	+= -Wall -Wextra -Wshadow -Wconversion -g
CXXFLAGS += -std=c++17
ifneq ($(shell uname),Darwin)
CFLAGS 	+= -fsanitize=leak -fsanitize=address -fsanitize=undefined
endif

.PHONY: all
all: build run

.PHONY: run
run: build
	$(BINDIR)/$(GEN)
	$(BINDIR)/$(PROG)

.PHONY: build
build: $(BINDIR)/$(PROG) $(BINDIR)/$(GEN)

$(BINDIR)/%.exe: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

$(BINDIR)/%.exe: $(SRCDIR)/%.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	$(RM) $(BINDIR)/$(PROG) $(BINDIR)/$(GEN)
