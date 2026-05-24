# Compiler settings
CC = gcc
CFLAGS = -O2 -Wall -Wextra $(shell pkg-config --cflags sdl2 SDL2_image)
LIBS = $(shell pkg-config --libs sdl2 SDL2_image)

# Project Structure
SRC = src/gsplash.c

# Package output path (default to pkg layout used in repository)
PKG_DIR ?= pkg/gsplash-git/usr
BIN_DIR := $(PKG_DIR)/bin
# Compiler settings
CC = gcc
CFLAGS = -O2 -Wall -Wextra $(shell pkg-config --cflags sdl2 SDL2_image)
LIBS = $(shell pkg-config --libs sdl2 SDL2_image)

SRC = src/gsplash.c

# Build output (temporary) and package layout
BUILD_DIR := build
BUILD_TARGET := $(BUILD_DIR)/gsplash

PKG_TOP ?= pkg/gsplash-git
PKG_DIR := $(PKG_TOP)/usr
BIN_DIR := $(PKG_DIR)/bin
PKG_TARGET := $(BIN_DIR)/game-splash

# Installation Paths (Defaults to user local bin)
PREFIX ?= /usr/local

# Default target: build the temporary artifact
all: $(BUILD_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Build rule: produce a temporary build artifact in build/
$(BUILD_TARGET): $(BUILD_DIR) $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BUILD_TARGET) $(LIBS)
	chmod 0755 $(BUILD_TARGET)

# Package: copy build artifact into pkg layout and create a tarball
.PHONY: package dist
package: $(BUILD_TARGET) $(BIN_DIR)
	cp -a $(BUILD_TARGET) $(PKG_TARGET)
	chmod 0755 $(PKG_TARGET)
	mkdir -p dist
	# Create a tarball of the package top-level directory
	tar -C pkg -czf dist/$(notdir $(PKG_TOP)).tar.gz $(notdir $(PKG_TOP))

dist: package

# Lightweight smoke test (headless via SDL_VIDEODRIVER=dummy)
.PHONY: check
check: $(BUILD_TARGET)
	@echo "Running smoke test (headless)..."
	# Run with a non-existent image and a noop target (/bin/true) - exits quickly
	SDL_VIDEODRIVER=dummy $(BUILD_TARGET) nonexistent.png /bin/true || true
	@echo "Smoke test finished"

# Install binary system-wide (installs as game-splash)
install: $(BUILD_TARGET)
	install -Dm755 $(BUILD_TARGET) $(DESTDIR)$(PREFIX)/bin/$(notdir $(PKG_TARGET))

# Remove binary from system
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(notdir $(PKG_TARGET))

# Clean build artifacts and package output
clean:
	rm -f $(BUILD_TARGET) $(PKG_TARGET) dist/$(notdir $(PKG_TOP)).tar.gz

.PHONY: all install uninstall clean
uninstall:
