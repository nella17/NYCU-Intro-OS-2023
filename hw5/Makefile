CXX		?= g++
NAME 	:= page-sim
TARGET	?= ./${NAME}.exe
SOURCE 	:= ${NAME}.cpp
CFLAGS	+= -Wall -Wextra -Wshadow -Wconversion -g -std=c++17
ifneq ($(shell uname),Darwin)
CFLAGS 	+= -fsanitize=leak -fsanitize=address -fsanitize=undefined
endif

.PHONY: all
all: build run

.PHONY: run
run: build files
	$(TARGET) whois.txt
	$(TARGET) zipf.txt

.PHONY: build
build: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CFLAGS) $^ -o $@

.PHONY: files
files: zipf.txt whois.txt

%.txt:
	wget "https://github.com/Sea-n/NCTU-110A-OS/raw/master/Assignment5-page/$@"

.PHONY: clean
clean:
	$(RM) $(TARGET)
