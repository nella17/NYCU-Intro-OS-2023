CC		?= gcc
BINDIR	?= ./
SRCDIR	?= ./

PROG 	:= main.exe
BF_LIB 	:= bf.so
FF_LIB 	:= ff.so
BINARY 	:= $(BINDIR)/$(PROG) $(BINDIR)/$(BF_LIB) $(BINDIR)/$(FF_LIB)

CFLAGS	+= -Wall -Wextra -Wshadow -Wconversion -g
ifeq ($(shell uname),Darwin)
LIBFLAG := -dynamiclib 
LOAD 	:= DYLD_INSERT_LIBRARIES
else
LIBFLAG := -shared
LOAD 	:= LD_PRELOAD
endif

.PHONY: all
all: build run

.PHONY: run
run: build
	$(BINDIR)/$(PROG)
	$(LOAD)=$(BINDIR)/$(BF_LIB) $(BINDIR)/$(PROG)
	$(LOAD)=$(BINDIR)/$(FF_LIB) $(BINDIR)/$(PROG)

.PHONY: build
build: $(BINARY)

$(BINDIR)/%.exe: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

$(BINDIR)/%.so: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(LIBFLAG) $< -o $@

.PHONY: clean
clean:
	$(RM) $(BINARY)
