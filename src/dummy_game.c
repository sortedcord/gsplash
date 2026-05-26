#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int delay_seconds = 5;
    if (argc > 1) {
        int parsed = atoi(argv[1]);
        if (parsed > 0) {
            delay_seconds = parsed;
        }
    }

    printf("[dummy_game] Simulating engine startup for %d seconds...\n", delay_seconds);
    for (int i = 0; i < delay_seconds; i++) {
        sleep(1);
        printf("[dummy_game] Loading... %d/%d\n", i + 1, delay_seconds);
    }

    printf("[dummy_game] Creating window to trigger focus-loss on splash screen...\n");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "[dummy_game] SDL Init Failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Dummy Game Window",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        fprintf(stderr, "[dummy_game] Window Creation Failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer) {
        SDL_SetRenderDrawColor(renderer, 0, 120, 255, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    printf("[dummy_game] Window created. Waiting 5 seconds before exiting...\n");

    SDL_Event event;
    int running = 1;
    Uint32 start_time = SDL_GetTicks();
    
    while (running && (SDL_GetTicks() - start_time < 5000)) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }
        SDL_Delay(50);
    }

    printf("[dummy_game] Exiting normally.\n");
    if (renderer) SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
