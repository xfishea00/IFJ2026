# Project: Implementace překladače imperativního jazyka IFJ26
# File:    Makefile
# Authors: Michal Řezníček (xreznim00)
#
# AI-generated (Claude Opus 5.5). Prompt: "priprav mi lokalne .gitignore
# a kostru Makefile pro projekt IFJ26 (make vytvori jeden
# binarni soubor, musi fungovat na Merlinu, formatovani mimo vychozi cil)"

CC = gcc
CFLAGS = -g -std=c11 -pedantic -Wall -Wextra
# -MMD -MP: generate .d files so that editing a header rebuilds its users
DEPFLAGS = -MMD -MP
TARGET = ifj2026

# All .c files in the root; new modules are picked up automatically
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
# Only files that actually exist (a bare *.c glob breaks when there are none)
FMT_SRCS = $(wildcard src/*.c src/*.h)

.PHONY: all build check format build-check valgrind asan clean

# Default target -- the only one run during evaluation
all: build

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(DEPS)

# Fails if any file is not formatted (needs clang-format, not on Merlin)
check:
	clang-format --dry-run --Werror $(FMT_SRCS)

# Reformats all sources in place
format:
	clang-format -i $(FMT_SRCS)

build-check: check build

# Usage: make valgrind < input.ifj26
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Separate binary built straight from the sources, so sanitized objects
# never mix with the normal build. Usage: ./ifj26-asan < input.ifj26
asan:
	$(CC) $(CFLAGS) -fsanitize=address,undefined $(SRCS) -o $(TARGET)-asan

clean:
	rm -f $(TARGET) $(TARGET)-asan $(OBJS) $(DEPS)
