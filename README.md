# gsplash

A fullscreen splash-screen wrapper for launching a game or app. It displays an image (or a fallback black screen), starts your executable, and closes when the process exits or loses focus after launch.

https://github.com/user-attachments/assets/22da49b4-0f1f-4208-8d0b-9eeef14e35e5

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

## Testing

Gsplash includes several testing utilities to ensure proper functionality without requiring a heavy game binary.

### Automated Testing

Run the automated test suite (which includes a headless smoke test and CLI argument validation) using:

```bash
make check
```

### Interactive Visual Testing

To physically test the splash screen rendering modes (`stretch`, `center`, `crop`) with a real image, use the interactive test script. It launches `gsplash` with each mode and prompts you to confirm if it displayed correctly:

```bash
./tests/test_interactive.sh path/to/your/image.png
```
*(Tip: You can place your test images in the `tests/assets/` directory).*

### Dummy Game Utility

For manual testing, a `dummy_game` binary is built alongside `gsplash`. It mimics a real game by sleeping to simulate startup time, creating an SDL window to trigger `gsplash`'s focus loss detection and exiting cleanly.

```bash
./build/gsplash path/to/image.png ./build/dummy_game

# Test with a custom 10 second simulated game load time
./build/gsplash path/to/image.png ./build/dummy_game 10
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
