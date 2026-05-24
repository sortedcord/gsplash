# Makefile for gsplash (community-standard layout)

PREFIX ?= /usr/local
bindir ?= $(PREFIX)/bin

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
PKG_CONFIG ?= pkg-config

SDL_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2 SDL2_image)
SDL_LIBS := $(shell $(PKG_CONFIG) --libs sdl2 SDL2_image)
FFMPEG_CFLAGS := $(shell $(PKG_CONFIG) --cflags libavformat libavcodec libswscale libavutil)
FFMPEG_LIBS := $(shell $(PKG_CONFIG) --libs libavformat libavcodec libswscale libavutil)

ALL_CFLAGS := $(CFLAGS) $(SDL_CFLAGS) $(FFMPEG_CFLAGS)
ALL_LIBS := $(LIBS) $(SDL_LIBS) $(FFMPEG_LIBS) -lm

BUILD_DIR ?= build
TARGET = $(BUILD_DIR)/gsplash
SRC = src/gsplash.c

.PHONY: all clean install uninstall check

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(BUILD_DIR) $(SRC)
	$(CC) $(ALL_CFLAGS) $(LDFLAGS) $(SRC) -o $@ $(ALL_LIBS)

# Lightweight smoke test (headless via SDL_VIDEODRIVER=dummy)
check: $(TARGET)
	@echo "Running smoke test (headless)..."
	SDL_VIDEODRIVER=dummy $(TARGET) nonexistent.png /bin/true || true
	@echo "Smoke test finished"

install: $(TARGET)
	install -d "$(DESTDIR)$(bindir)"
	install -m 755 $(TARGET) "$(DESTDIR)$(bindir)/gsplash"

uninstall:
	rm -f "$(DESTDIR)$(bindir)/gsplash"

clean:
	rm -f $(TARGET)
