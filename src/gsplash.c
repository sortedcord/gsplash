#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

static void log_info(const char *fmt, ...)
{
    va_list args;
    fprintf(stderr, "[splash] ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
    fflush(stderr);
}

static void log_error(const char *fmt, ...)
{
    va_list args;
    fprintf(stderr, "[splash][error] ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
    fflush(stderr);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        log_error("Usage: %s <image_path> <game_executable> [args...]", argv[0]);
        return 1;
    }

    log_info("Starting splash: image='%s', game='%s'", argv[1], argv[2]);

    // Initialize SDL2 Video subsystems
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        log_error("SDL Init Failed: %s", SDL_GetError());
        return 1;
    }
    log_info("SDL initialized");

    // Initialize JPEG and PNG decoders
    if (!(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) & (IMG_INIT_JPG | IMG_INIT_PNG)))
    {
        log_error("SDL_image Init Failed: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }
    log_info("SDL_image initialized (JPG/PNG)");

    // Create a native borderless fullscreen window
    SDL_Window *window = SDL_CreateWindow(
        "Game Splash Screen",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        0, 0,
        SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS);

    if (!window)
    {
        log_error("Window Creation Failed: %s", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    log_info("Fullscreen borderless window created");

    SDL_ShowCursor(SDL_DISABLE); // Hide the mouse pointer

    // Create a hardware-accelerated renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer)
    {
        log_info("Renderer created (accelerated)");
    }
    SDL_Texture *texture = IMG_LoadTexture(renderer, argv[1]);

    if (!texture)
    {
        log_error("Failed to load splash image '%s': %s; showing black screen", argv[1], IMG_GetError());
        // Fallback: Clear to solid black if image file is broken
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }
    else
    {
        log_info("Splash image loaded");
        SDL_RenderClear(renderer);
        // Automatically scales your image to perfectly match the monitor aspect ratio
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    // Fork the process to run the game
    log_info("Launching game executable");
    pid_t pid = fork();
    if (pid == 0)
    {
        // Inside Child Process: Hand over execution directly to the game binary
        log_info("Execing game: %s", argv[2]);
        execvp(argv[2], &argv[2]);
        log_error("Failed to launch target game executable '%s': %s", argv[2], strerror(errno));
        _exit(1);
    }
    else if (pid < 0)
    {
        log_error("Failed to fork process: %s", strerror(errno));
    }
    else
    {
        log_info("Game process started (pid=%d)", pid);
        // Inside Parent Process: Manage splash screen lifecycle
        int running = 1;
        SDL_Event event;

        while (running)
        {
            // Non-blocking check: Has the game quit?
            int status;
            pid_t result = waitpid(pid, &status, WNOHANG);
            if (result > 0)
            {
                if (WIFEXITED(status))
                {
                    log_info("Game exited (code=%d)", WEXITSTATUS(status));
                }
                else if (WIFSIGNALED(status))
                {
                    log_info("Game terminated by signal %d", WTERMSIG(status));
                }
                else
                {
                    log_info("Game exited");
                }
                break; // Game closed, break the loop and close the script
            }
            else if (result < 0)
            {
                log_error("waitpid failed: %s", strerror(errno));
                break; // Process error safetynet
            }

            // Check desktop server window events
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                {
                    log_info("Splash dismissed via window close");
                    running = 0;
                }
                if (event.type == SDL_WINDOWEVENT)
                {
                    // THE MOMENT THE GAME WINDOW STEALS FOCUS:
                    if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
                    {
                        log_info("Splash window hidden (focus lost)");
                        SDL_HideWindow(window); // Instantly make splash transparent/invisible
                    }
                }
                if (event.type == SDL_KEYDOWN)
                {
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        log_info("Splash dismissed via escape key");
                        running = 0; // Emergency escape key loop override
                    }
                }
            }
            SDL_Delay(33); // ~30 FPS polling loop to ensure near-zero CPU usage
        }
    }

    // Clean up memory space
    if (texture)
        SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    log_info("Splash shutdown complete");
    return 0;
}
