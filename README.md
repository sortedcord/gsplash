# gsplash

A fullscreen splash-screen wrapper for launching a game or app. It displays an image (or a fallback black screen), starts your executable, and closes when the process exits or loses focus after launch.

## Features

1. Fullscreen, borderless splash screen with hidden cursor (SDL2)
2. Displays a supplied image via SDL2_image (fallback to black if load fails)
3. Launches the target executable and exits when it finishes
4. Hides on focus loss after the game starts or closes on **Esc**

## Build Requirements

- C compiler (GCC or Clang)
- make
- pkg-config
- SDL2
- SDL2_image

## Install

Arch Linux (PKGBUILD) for integrating with your system package manager:

```bash
makepkg -si
```

For other distributions, build and install manually:

```bash
make
sudo make install
```

## Usage

Build output installs to `/usr/local/bin` by default. Override with `PREFIX`.

```bash
game-splash <image_path> <game_executable> [game_arguments...]
```

```bash
game-splash assets/splash.jpg /path/to/game --fullscreen --profile=default
```
