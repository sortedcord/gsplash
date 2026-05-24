# gsplash

A fullscreen splash-screen wrapper for launching a game or app. It displays an image (or a fallback text label), starts your executable, and closes when the process exits or loses focus after launch.

## Features

1. Fullscreen, borderless splash screen with hidden cursor
2. Displays a supplied image via Pillow (fallback text if load fails)
3. Launches the target executable and exits when it finishes
4. Closes on focus loss after the game starts or on **Esc**

## Requirements

- Python 3.12+
- Tkinter (usually bundled with Python; on some Linux distros you may need a `tk` system package)
- Pillow

## Install

Using `uv` (recommended):

```bash
uv sync
```

Using `pip`:

```bash
python -m venv .venv
source .venv/bin/activate
pip install .
```

## Usage

```bash
python main.py <path_to_image> <game_executable> [game_arguments...]
```

Example:

```bash
python main.py assets/splash.jpg /path/to/game --fullscreen --profile=default
```
