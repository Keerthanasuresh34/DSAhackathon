# ====================================================================
# The Museum's Vanishing Family Trees — Archiving & Recovery System
# Unified C Engine Makefile (Cross-Platform: Windows, Linux, macOS)
# ====================================================================

CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -O2

# Detect Windows vs POSIX
ifeq ($(OS),Windows_NT)
    TARGET = tree_engine.exe
    RM_CMD = del /Q /F 2>nul || exit 0
else
    TARGET = tree_engine
    RM_CMD = rm -f
endif

SRCS = tree.c archive.c main.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

tree.o: tree.c tree.h
	$(CC) $(CFLAGS) -c tree.c -o tree.o

archive.o: archive.c archive.h tree.h
	$(CC) $(CFLAGS) -c archive.c -o archive.o

main.o: main.c tree.h archive.h
	$(CC) $(CFLAGS) -c main.c -o main.o

clean:
	$(RM_CMD) $(OBJS) $(TARGET)
