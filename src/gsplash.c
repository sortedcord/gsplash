#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <ctype.h>

#include "video.h"

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

typedef enum RenderMode
{
    RENDER_STRETCH = 0,
    RENDER_CENTER,
    RENDER_CROP
} RenderMode;

static RenderMode parse_render_mode(const char *value)
{
    if (strcmp(value, "center") == 0)
    {
        return RENDER_CENTER;
    }
    if (strcmp(value, "crop") == 0)
    {
        return RENDER_CROP;
    }
    return RENDER_STRETCH;
}

static void compute_dest_rect(int src_w, int src_h, int out_w, int out_h, RenderMode mode, SDL_Rect *dst)
{
    dst->x = 0;
    dst->y = 0;
    dst->w = out_w;
    dst->h = out_h;

    if (src_w <= 0 || src_h <= 0 || out_w <= 0 || out_h <= 0)
    {
        return;
    }

    if (mode == RENDER_CENTER)
    {
        float scale = fminf((float)out_w / (float)src_w, (float)out_h / (float)src_h);
        dst->w = (int)(src_w * scale);
        dst->h = (int)(src_h * scale);
        dst->x = (out_w - dst->w) / 2;
        dst->y = (out_h - dst->h) / 2;
    }
    else if (mode == RENDER_CROP)
    {
        float scale = fmaxf((float)out_w / (float)src_w, (float)out_h / (float)src_h);
        dst->w = (int)(src_w * scale);
        dst->h = (int)(src_h * scale);
        dst->x = (out_w - dst->w) / 2;
        dst->y = (out_h - dst->h) / 2;
    }
}

int main(int argc, char *argv[])
{
    RenderMode render_mode = RENDER_CENTER;
    int arg_index = 1;

    if (argc >= 3 && strncmp(argv[1], "--mode=", 7) == 0)
    {
        render_mode = parse_render_mode(argv[1] + 7);
        arg_index += 1;
    }
    else if (argc >= 2 && strcmp(argv[1], "-m") == 0)
    {
        if (argc < 4)
        {
            log_error("Usage: %s [--mode=stretch|center|crop] <image_path> <game_executable> [args...]", argv[0]);
            log_error("       %s -m stretch|center|crop <image_path> <game_executable> [args...]", argv[0]);
            return 1;
        }
        render_mode = parse_render_mode(argv[2]);
        arg_index += 2;
    }

    if (argc - arg_index < 2)
    {
        log_error("Usage: %s [--mode=stretch|center|crop] <image_path> <game_executable> [args...]", argv[0]);
        log_error("       %s -m stretch|center|crop <image_path> <game_executable> [args...]", argv[0]);
        return 1;
    }

    const char *image_path = argv[arg_index];
    const char *game_path = argv[arg_index + 1];

    log_info("Starting splash: image='%s', game='%s', mode=%d", image_path, game_path, render_mode);

    // Initialize SDL2 Video subsystems
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        log_error("SDL Init Failed: %s", SDL_GetError());
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
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
    SDL_Texture *texture = NULL;
    VideoPlayer video_player;
    bool video_active = false;

    if (has_video_extension(image_path))
    {
        if (init_video_player(&video_player, renderer, image_path))
        {
            video_active = true;
        }
        else
        {
            log_error("Failed to open video '%s'; showing black screen", image_path);
        }
    }
    else
    {
        SDL_Surface *surface = IMG_Load(image_path);
        if (surface)
        {
            SDL_Surface *rgba_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
            SDL_FreeSurface(surface);
            if (rgba_surface)
            {
                texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                            SDL_TEXTUREACCESS_STREAMING,
                                            rgba_surface->w, rgba_surface->h);
                if (texture)
                {
                    SDL_UpdateTexture(texture, NULL, rgba_surface->pixels, rgba_surface->pitch);
                }
                else
                {
                    log_error("Failed to create texture: %s", SDL_GetError());
                }
                SDL_FreeSurface(rgba_surface);
            }
        }
        if (!texture && init_video_player(&video_player, renderer, image_path))
        {
            video_active = true;
        }
    }

    if (!texture && !video_active)
    {
        log_error("Failed to load splash image '%s': %s; showing black screen", image_path, IMG_GetError());
        // Fallback: Clear to solid black if image file is broken
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }
    else
    {
        SDL_Rect dst_rect = {0, 0, 0, 0};
        int out_w = 0;
        int out_h = 0;

        SDL_GetRendererOutputSize(renderer, &out_w, &out_h);

        if (video_active)
        {
            log_info("Video splash loaded");
            compute_dest_rect(video_player.width, video_player.height, out_w, out_h, render_mode, &dst_rect);
            decode_next_frame(&video_player);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, video_player.texture, NULL, &dst_rect);
            SDL_RenderPresent(renderer);
        }
        else if (texture)
        {
            int tex_w = 0;
            int tex_h = 0;

            log_info("Splash image loaded");
            if (SDL_QueryTexture(texture, NULL, NULL, &tex_w, &tex_h) == 0)
            {
                compute_dest_rect(tex_w, tex_h, out_w, out_h, render_mode, &dst_rect);
            }

            SDL_RenderClear(renderer);
            if (dst_rect.w > 0 && dst_rect.h > 0)
            {
                SDL_RenderCopy(renderer, texture, NULL, &dst_rect);
            }
            else
            {
                SDL_RenderCopy(renderer, texture, NULL, NULL);
            }
            SDL_RenderPresent(renderer);
        }
    }

    // Fork the process to run the game
    log_info("Launching game executable");
    pid_t pid = fork();
    if (pid == 0)
    {
        // Inside Child Process: Hand over execution directly to the game binary
        log_info("Execing game: %s", game_path);
        execvp(game_path, &argv[arg_index + 1]);
        log_error("Failed to launch target game executable '%s': %s", game_path, strerror(errno));
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
        bool hide_scheduled = false;
        Uint32 hide_time = 0;

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

            if (video_active && SDL_GetTicks() >= video_player.next_frame_tick)
            {
                if (!decode_next_frame(&video_player))
                {
                    // If decoding fails or loops, try again on next tick
                }
                video_player.next_frame_tick += (Uint32)video_player.frame_delay_ms;
                Uint32 now = SDL_GetTicks();
                if ((Sint32)(now - video_player.next_frame_tick) > 100)
                {
                    video_player.next_frame_tick = now;
                }

                SDL_RenderClear(renderer);
                SDL_Rect dst_rect = {0, 0, 0, 0};
                int out_w = 0;
                int out_h = 0;
                SDL_GetRendererOutputSize(renderer, &out_w, &out_h);
                compute_dest_rect(video_player.width, video_player.height, out_w, out_h, render_mode, &dst_rect);
                SDL_RenderCopy(renderer, video_player.texture, NULL, &dst_rect);
                SDL_RenderPresent(renderer);
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
                    if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST && !hide_scheduled)
                    {
                        log_info("Splash window hidden (focus lost), scheduling hide");
                        hide_scheduled = true;
                        hide_time = SDL_GetTicks() + 250; // 250ms delay to prevent desktop flash
                    }
                    // Re-render static image when compositor requests a redraw
                    if (event.window.event == SDL_WINDOWEVENT_EXPOSED && texture && !video_active)
                    {
                        int out_w = 0, out_h = 0;
                        SDL_GetRendererOutputSize(renderer, &out_w, &out_h);
                        int tex_w = 0, tex_h = 0;
                        SDL_QueryTexture(texture, NULL, NULL, &tex_w, &tex_h);
                        SDL_Rect dst_rect = {0, 0, 0, 0};
                        compute_dest_rect(tex_w, tex_h, out_w, out_h, render_mode, &dst_rect);
                        SDL_RenderClear(renderer);
                        if (dst_rect.w > 0 && dst_rect.h > 0)
                            SDL_RenderCopy(renderer, texture, NULL, &dst_rect);
                        else
                            SDL_RenderCopy(renderer, texture, NULL, NULL);
                        SDL_RenderPresent(renderer);
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

            if (hide_scheduled && SDL_GetTicks() >= hide_time)
            {
                log_info("Hiding splash window (delayed)");
                SDL_HideWindow(window);
                hide_scheduled = false;
            }

            if (video_active)
            {
                Uint32 now = SDL_GetTicks();
                if ((Sint32)(video_player.next_frame_tick - now) > 0)
                {
                    Uint32 delay = video_player.next_frame_tick - now;
                    SDL_Delay(delay > 33 ? 33 : delay);
                }
            }
            else
            {
                SDL_Delay(33); // ~30 FPS polling loop to ensure near-zero CPU usage
            }
        }
    }

    // Clean up memory space
    if (texture)
        SDL_DestroyTexture(texture);
    if (video_active)
        cleanup_video_player(&video_player);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    log_info("Splash shutdown complete");
    return 0;
}
