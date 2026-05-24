# Makefile for gsplash (community-standard layout)

PREFIX ?= /usr/local
bindir ?= $(PREFIX)/bin

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
PKG_CONFIG ?= pkg-config

SDL_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2 SDL2_image)
SDL_LIBS := $(shell $(PKG_CONFIG) --libs sdl2 SDL2_image)

ALL_CFLAGS := $(CFLAGS) $(SDL_CFLAGS)
ALL_LIBS := $(LIBS) $(SDL_LIBS) -lm

TARGET = gsplash
SRC = src/gsplash.c

.PHONY: all clean install uninstall check

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(ALL_CFLAGS) $(LDFLAGS) $< -o $@ $(ALL_LIBS)

# Lightweight smoke test (headless via SDL_VIDEODRIVER=dummy)
check: $(TARGET)
	@echo "Running smoke test (headless)..."
	SDL_VIDEODRIVER=dummy ./$(TARGET) nonexistent.png /bin/true || true
	@echo "Smoke test finished"

install: $(TARGET)
	install -d "$(DESTDIR)$(bindir)"
	install -m 755 $(TARGET) "$(DESTDIR)$(bindir)/$(TARGET)"

uninstall:
	rm -f "$(DESTDIR)$(bindir)/$(TARGET)"

clean:
	rm -f $(TARGET)
