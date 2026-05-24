# Compiler settings
CC = gcc
CFLAGS = -O2 -Wall -Wextra $(shell pkg-config --cflags sdl2 SDL2_image)
LIBS = $(shell pkg-config --libs sdl2 SDL2_image)

# Project Structure
TARGET = game-splash
SRC = src/gsplash.c

# Installation Paths (Defaults to user local bin)
PREFIX ?= /usr/local

# Default target run when typing just 'make'
all: $(TARGET)

# Compilation rule
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

# Install binary system-wide
install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

# Remove binary from system
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

# Clean build artifacts
clean:
	rm -f $(TARGET)

.PHONY: all install uninstall clean
