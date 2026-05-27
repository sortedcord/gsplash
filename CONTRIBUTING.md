# Contributing to gsplash

Thanks for your interest in contributing. gsplash is a small utility and contributions that keep it that way are most welcome.

## What to work on

Check the issue tracker on [GitHub](https://github.com/sortedcord/gsplash) for open issues. If you want to add something new, open an issue first to discuss it before writing code. This avoids wasted effort if the change isn't a good fit for the project.

Good areas to contribute:

- Bug fixes (see known issues in README or open issues)
- Broader video format or image format support
- Wayland compatibility improvements
- Packaging for other distributions

## Building

You'll need:

- GCC or Clang
- make, pkg-config
- SDL2, SDL2\_image
- ffmpeg (`libavformat`, `libavcodec`, `libswscale`, `libavutil`)

On Arch Linux:

```bash
sudo pacman -S sdl2 sdl2_image ffmpeg
```

On Ubuntu/Debian:

```bash
sudo apt install libsdl2-dev libsdl2-image-dev libavformat-dev libavcodec-dev libswscale-dev libavutil-dev
```

Then build:

```bash
make
```

The binary is output to `build/gsplash`.

## Testing

Before submitting a PR, run the automated test suite:

```bash
make check
```

This runs a headless smoke test and the CLI argument validation suite. Both must pass.

For visual changes (render modes, layout), also run the interactive test with a real image:

```bash
./tests/test_interactive.sh path/to/image.png
```

If you're adding new CLI arguments or flags, add corresponding cases to `tests/test_cli.sh`.

## Code style

- Match the existing style: 4-space indentation, `snake_case`, braces on their own line.
- All new functions in `video.c`/`video.h` should have declarations in the header.
- Avoid adding new dependencies unless there's a strong reason. SDL2 and ffmpeg are already heavy. 

## Submitting a PR

1. Fork the repo and create a branch from `main`.
2. Keep commits focused — one logical change per commit.
3. Write a clear PR description explaining what the change does and why.
4. If the change fixes a bug, reference the issue number.
5. PRs should target `github.com/sortedcord/gsplash` — this is the canonical public remote.

## Reporting bugs

Open an issue on GitHub with:

- Your OS and desktop environment
- The gsplash version or commit hash
- The command you ran
- The full stderr output (gsplash logs to stderr via `[splash]` prefixed lines)