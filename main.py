#!/usr/bin/env python3
import sys
import subprocess
import threading
import time
import tkinter as tk
from PIL import Image, ImageTk


def main():
    if len(sys.argv) < 3:
        print(
            "Usage: splash_wrapper.py <path_to_image> <game_executable> [game_arguments...]"
        )
        sys.exit(1)

    image_path = sys.argv[1]
    game_command = sys.argv[2:]

    root = tk.Tk()
    root.title("Game Splash Screen")
    root.attributes("-fullscreen", True)
    root.configure(bg="black")
    root.config(cursor="none")

    try:
        pil_img = Image.open(image_path)
        splash_img = ImageTk.PhotoImage(pil_img)
        label = tk.Label(root, image=splash_img, bg="black")
        label.image = splash_img
        label.pack(expand=True)
    except Exception as e:
        print(f"Error loading image: {e}. Falling back to text.")
        label = tk.Label(
            root, text="Loading Game...", fg="white", bg="black", font=("Helvetica", 28)
        )
        label.pack(expand=True)

    state = {"game_started": False}

    def monitor_game():
        time.sleep(0.5)
        # Launch the game
        process = subprocess.Popen(game_command)
        state["game_started"] = True

        # Keep this script alive as long as the game process is running
        while process.poll() is None:
            time.sleep(0.1)

        # THE GAME HAS QUIT: Now safely shut down the script entirely
        print("Game closed. Exiting wrapper script.")
        root.after(0, root.destroy)

    # Start the monitoring thread
    threading.Thread(target=monitor_game, daemon=True).start()

    def on_focus_out(event):
        if state["game_started"]:
            # THE GAME WINDOW SHOWED UP: Hide the splash window instead of closing the script
            root.after(300, root.withdraw)

    root.bind("<FocusOut>", on_focus_out)
    root.bind("<Escape>", lambda e: root.destroy())

    root.mainloop()


if __name__ == "__main__":
    main()
