# gsplash

A fullscreen splash-screen wrapper for launching a game or app. It displays an image (or a fallback black screen), starts your executable, and closes when the process exits or loses focus after launch.

## Features

1. Fullscreen, borderless splash screen with hidden cursor (SDL2)
2. Displays a supplied image via SDL2_image or video via ffmpeg (fallback to black if load fails)
3. Launches the target executable and exits when it finishes
4. Hides on focus loss after the game starts or closes on **Esc**

## Build Requirements

- C compiler (GCC or Clang)
- make
- pkg-config
- SDL2
- SDL2_image
- ffmpeg (libavformat, libavcodec, libswscale, libavutil)

## Install

Arch Linux (PKGBUILD) for integrating with your system package manager:

```bash
makepkg -si
```

For other distributions, build and install manually:

```bash
# Build the binary into build/
make

# Install system-wide (defaults to /usr/local)
sudo make install

# Staged install (useful for packaging):
DESTDIR=/some/staging/path make install
```

## Smoke test (headless)

Run a lightweight headless smoke test that uses the dummy video driver so it doesn't require a display:

```bash
make check
```

## Usage

```bash
gsplash <image_or_video_path> <game_executable> [game_arguments...]
```

Example:

```bash
gsplash assets/splash.jpg /path/to/game --fullscreen --profile=default

# Video splash (supported formats depend on ffmpeg build)
gsplash assets/splash.mp4 /path/to/game --fullscreen --profile=default
```

Gsplash allows you to configure how the image or video is displayed with 3 modes:

- `center` (default): letterbox
- `crop`: fill screen by cropping
- `stretch`: Distort to fill screen

You can set these by using the `-m` or `--mode` flag:

```bash
build/gsplash [--mode=stretch|center|crop] <background> <executable> [args...]

build/gsplash -m stretch|center|crop <background> <executable> [args...]
```