CC		?= gcc
BINDIR	?= ./
SRCDIR	?= ./

PROG 	:= main.exe
BF_LIB 	:= bf.so
FF_LIB 	:= ff.so
BINARY 	:= $(BINDIR)/$(PROG) $(BINDIR)/$(BF_LIB) $(BINDIR)/$(FF_LIB)

CFLAGS	+= -Wall -Wextra -Wshadow -Wconversion -g
ifneq ($(shell uname),Darwin)
CFLAGS 	+= -fsanitize=leak -fsanitize=address -fsanitize=undefined
endif

.PHONY: all
all: build run

.PHONY: run
run: build
	$(BINDIR)/$(PROG)
	LD_PRELOAD=$(BINDIR)/$(BF_LIB) $(BINDIR)/$(PROG)
	LD_PRELOAD=$(BINDIR)/$(FF_LIB) $(BINDIR)/$(PROG)

.PHONY: build
build: $(BINARY)

$(BINDIR)/%.exe: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

$(BINDIR)/%.so: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -shared $< -o $@

.PHONY: clean
clean:
	$(RM) $(BINARY)
