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
all: build test

.PHONY: run
run: build testdir
	$(TARGET) ./test

.PHONY: test
test: build
	./demo.sh $(TARGET)

.PHONY: testdir
testdir:
	rm -fr $(TESTDIR) && \
	mkdir $(TESTDIR) && \
	tar -xf $(TESTCASE) --directory=$(TESTDIR) \

.PHONY: build
build: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $^ -o $@ $(CPPFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	$(RM) $(TARGET)
