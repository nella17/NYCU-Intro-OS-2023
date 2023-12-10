CXX		?= g++
NAME 	:= dedup
TARGET	?= ./${NAME}.exe
SOURCE 	:= ${NAME}.cpp
CPPFLAGS += -Wall -Wextra -Wshadow -Wconversion -g -std=c++17 -lssl -lcrypto
ifneq ($(shell uname),Darwin)
CPPFLAGS += -fsanitize=leak -fsanitize=address -fsanitize=undefined
endif

TESTCASE ?= test.tar.xz
TESTDIR ?= test
ANSDIR 	?= answer

.PHONY: all
all: build run

.PHONY: run
run: build testdir
	$(TARGET) ./test

.PHONY: testdir
testdir:
	diff -rq $(TESTDIR) $(ANSDIR) >/dev/null || \
		( \
			rm -fr $(TESTDIR) && \
			mkdir $(TESTDIR) && \
			tar -xf $(TESTCASE) --directory=$(TESTDIR) \
		)

.PHONY: build
build: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

.PHONY: clean
clean:
	$(RM) $(TARGET)
