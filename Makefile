SHELL := /bin/bash

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -g
# Source files
SRC = $(wildcard *.c)
# Target executable name
TARGET = $(patsubst %.c,%,$(wildcard *.c))

# Default target
all: $(TARGET) .gitignore .gitignore2

# Compile source to executable, object file, assembler file, and preprocessed file
%: %.c
	@$(CC) $(CFLAGS) -o $@ $<
	# @$(CC) $(CFLAGS) -S -o $@.s $<

# Generate .gitignore
.gitignore: $(TARGET)
	@echo $(TARGET) | tr ' ' '\n' > .gitignore

.gitignore2:
	@echo ".vscode .DS_Store .bin .out" | tr ' ' '\n' >> .gitignore

# Clean up build files
clean:
	rm -f $(TARGET) $(TARGET:=.o) $(TARGET:=.s) $(TARGET:=.i) .gitignore

# Phony targets
.PHONY: all clean .gitignore