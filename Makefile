CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g
TARGET = ifj2026
SRC_DIR = src

SRCS = $(wildcard$(SRC_DIR)/*.c)
OBJS = $(SRCS:.c=.o)

.PHONY: all clean format

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

format:
	clang-format -i src/*.c src/*.h

clean:
	rm -f $(OBJS) $(TARGET)