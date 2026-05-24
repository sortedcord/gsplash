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

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

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

typedef struct VideoPlayer
{
    AVFormatContext *format_ctx;
    AVCodecContext *codec_ctx;
    struct SwsContext *sws_ctx;
    AVFrame *frame;
    AVFrame *rgba_frame;
    uint8_t *rgba_buffer;
    int rgba_buffer_size;
    int stream_index;
    int width;
    int height;
    int frame_delay_ms;
    Uint32 next_frame_tick;
    SDL_Texture *texture;
} VideoPlayer;

static bool has_video_extension(const char *path)
{
    const char *dot = strrchr(path, '.');
    if (!dot || dot == path)
    {
        return false;
    }

    char ext[8];
    size_t i = 0;
    dot++;
    while (*dot && i < sizeof(ext) - 1)
    {
        ext[i++] = (char)tolower((unsigned char)*dot++);
    }
    ext[i] = '\0';

    return strcmp(ext, "mp4") == 0 || strcmp(ext, "mkv") == 0 || strcmp(ext, "webm") == 0 ||
           strcmp(ext, "avi") == 0 || strcmp(ext, "mov") == 0 || strcmp(ext, "mpg") == 0 ||
           strcmp(ext, "mpeg") == 0;
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

static bool init_video_player(VideoPlayer *player, SDL_Renderer *renderer, const char *path)
{
    memset(player, 0, sizeof(*player));

    if (avformat_open_input(&player->format_ctx, path, NULL, NULL) != 0)
    {
        return false;
    }

    if (avformat_find_stream_info(player->format_ctx, NULL) < 0)
    {
        return false;
    }

    player->stream_index = -1;
    for (unsigned int i = 0; i < player->format_ctx->nb_streams; ++i)
    {
        if (player->format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            player->stream_index = (int)i;
            break;
        }
    }

    if (player->stream_index < 0)
    {
        return false;
    }

    AVCodecParameters *codecpar = player->format_ctx->streams[player->stream_index]->codecpar;
    const AVCodec *decoder = avcodec_find_decoder(codecpar->codec_id);
    if (!decoder)
    {
        return false;
    }

    player->codec_ctx = avcodec_alloc_context3(decoder);
    if (!player->codec_ctx)
    {
        return false;
    }

    if (avcodec_parameters_to_context(player->codec_ctx, codecpar) < 0)
    {
        return false;
    }

    if (avcodec_open2(player->codec_ctx, decoder, NULL) < 0)
    {
        return false;
    }

    player->width = player->codec_ctx->width;
    player->height = player->codec_ctx->height;
    player->frame = av_frame_alloc();
    player->rgba_frame = av_frame_alloc();
    if (!player->frame || !player->rgba_frame)
    {
        return false;
    }

    player->rgba_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, player->width, player->height, 1);
    player->rgba_buffer = (uint8_t *)av_malloc(player->rgba_buffer_size);
    if (!player->rgba_buffer)
    {
        return false;
    }

    av_image_fill_arrays(player->rgba_frame->data, player->rgba_frame->linesize, player->rgba_buffer,
                         AV_PIX_FMT_RGBA, player->width, player->height, 1);

    player->sws_ctx = sws_getContext(player->width, player->height, player->codec_ctx->pix_fmt,
                                     player->width, player->height, AV_PIX_FMT_RGBA, SWS_BILINEAR,
                                     NULL, NULL, NULL);
    if (!player->sws_ctx)
    {
        return false;
    }

    player->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                        SDL_TEXTUREACCESS_STREAMING, player->width, player->height);
    if (!player->texture)
    {
        return false;
    }

    AVRational fr = player->format_ctx->streams[player->stream_index]->avg_frame_rate;
    if (fr.num > 0 && fr.den > 0)
    {
        player->frame_delay_ms = (int)(1000.0 * fr.den / fr.num);
    }
    else
    {
        player->frame_delay_ms = 33;
    }

    player->next_frame_tick = SDL_GetTicks();
    return true;
}

static void cleanup_video_player(VideoPlayer *player)
{
    if (player->texture)
    {
        SDL_DestroyTexture(player->texture);
    }
    if (player->sws_ctx)
    {
        sws_freeContext(player->sws_ctx);
    }
    if (player->rgba_buffer)
    {
        av_free(player->rgba_buffer);
    }
    if (player->rgba_frame)
    {
        av_frame_free(&player->rgba_frame);
    }
    if (player->frame)
    {
        av_frame_free(&player->frame);
    }
    if (player->codec_ctx)
    {
        avcodec_free_context(&player->codec_ctx);
    }
    if (player->format_ctx)
    {
        avformat_close_input(&player->format_ctx);
    }
    memset(player, 0, sizeof(*player));
}

static bool decode_next_frame(VideoPlayer *player)
{
    AVPacket packet;
    av_init_packet(&packet);

    while (av_read_frame(player->format_ctx, &packet) >= 0)
    {
        if (packet.stream_index != player->stream_index)
        {
            av_packet_unref(&packet);
            continue;
        }

        if (avcodec_send_packet(player->codec_ctx, &packet) < 0)
        {
            av_packet_unref(&packet);
            continue;
        }
        av_packet_unref(&packet);

        while (true)
        {
            int ret = avcodec_receive_frame(player->codec_ctx, player->frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                break;
            }
            if (ret < 0)
            {
                return false;
            }

            sws_scale(player->sws_ctx, (const uint8_t *const *)player->frame->data,
                      player->frame->linesize, 0, player->height,
                      player->rgba_frame->data, player->rgba_frame->linesize);

            SDL_UpdateTexture(player->texture, NULL, player->rgba_frame->data[0],
                              player->rgba_frame->linesize[0]);
            return true;
        }
    }

    av_seek_frame(player->format_ctx, player->stream_index, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(player->codec_ctx);
    return false;
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
    else if (argc >= 4 && strcmp(argv[1], "-m") == 0)
    {
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

    log_info("Starting splash: image='%s', game='%s'", image_path, game_path);

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
        texture = IMG_LoadTexture(renderer, image_path);
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
                video_player.next_frame_tick = SDL_GetTicks() + (Uint32)video_player.frame_delay_ms;

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
    if (video_active)
        cleanup_video_player(&video_player);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    log_info("Splash shutdown complete");
    return 0;
}
