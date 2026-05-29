# Makefile for gsplash (community-standard layout)

VERSION ?= $(shell git describe --tags --always --dirty 2>/dev/null || echo "unknown")

PREFIX ?= /usr/local
bindir ?= $(PREFIX)/bin

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
CFLAGS += -DGSPLASH_VERSION=\"$(VERSION)\"
PKG_CONFIG ?= pkg-config

SDL_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2 SDL2_image)
SDL_LIBS := $(shell $(PKG_CONFIG) --libs sdl2 SDL2_image)
FFMPEG_CFLAGS := $(shell $(PKG_CONFIG) --cflags libavformat libavcodec libswscale libswresample libavutil)
FFMPEG_LIBS := $(shell $(PKG_CONFIG) --libs libavformat libavcodec libswscale libswresample libavutil)

ALL_CFLAGS := $(CFLAGS) $(SDL_CFLAGS) $(FFMPEG_CFLAGS)
ALL_LIBS := $(LIBS) $(SDL_LIBS) $(FFMPEG_LIBS) -lm

BUILD_DIR ?= build
TARGET = $(BUILD_DIR)/gsplash
DUMMY_TARGET = $(BUILD_DIR)/dummy_game
SRC = src/gsplash.c src/video.c src/audio.c
DUMMY_SRC = src/dummy_game.c

.PHONY: all clean install uninstall check

all: $(TARGET) $(DUMMY_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(BUILD_DIR) $(SRC)
	$(CC) $(ALL_CFLAGS) $(LDFLAGS) $(SRC) -o $@ $(ALL_LIBS)

$(DUMMY_TARGET): $(BUILD_DIR) $(DUMMY_SRC)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(LDFLAGS) $(DUMMY_SRC) -o $@ $(SDL_LIBS)

# Lightweight smoke test (headless via SDL_VIDEODRIVER=dummy)
check: $(TARGET) $(DUMMY_TARGET)
	@echo "Running smoke test (headless)..."
	SDL_VIDEODRIVER=dummy $(TARGET) nonexistent.png /bin/true || true
	@echo "Running CLI test suite..."
	./tests/test_cli.sh
	@echo "All tests finished successfully"

install: $(TARGET)
	install -d "$(DESTDIR)$(bindir)"
	install -m 755 $(TARGET) "$(DESTDIR)$(bindir)/gsplash"

uninstall:
	rm -f "$(DESTDIR)$(bindir)/gsplash"

clean:
	rm -f $(TARGET) $(DUMMY_TARGET)
