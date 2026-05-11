#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <iostream>
#include <cstdlib>
#include <ctime>

#include "defs.h"
#include "game.h"

int SDL_main(int /*argc*/, char* /*argv*/[]) {
    std::srand((unsigned)std::time(nullptr));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL_Init error: " << SDL_GetError() << "\n";
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "IMG_Init error: " << IMG_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Mix_OpenAudio warning: " << Mix_GetError()
                  << " (continuing without audio)\n";
    }

    Mix_Init(MIX_INIT_MP3);

    SDL_Window* window = SDL_CreateWindow(
        "Chromium Dino Game",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        std::cerr << "SDL_CreateWindow error: " << SDL_GetError() << "\n";
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer error: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

    SDL_Surface* surf = IMG_Load(
        "resources/images/default_100_percent/offline/100-offline-sprite.png");
    if (!surf) {
        std::cerr << "IMG_Load error: " << IMG_GetError() << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* sprite = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    if (!sprite) {
        std::cerr << "SDL_CreateTextureFromSurface error: " << SDL_GetError() << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_SetTextureBlendMode(sprite, SDL_BLENDMODE_BLEND);

    Game game(renderer, sprite);

    constexpr Uint32 FRAME_DELAY = 1000 / FPS;
    SDL_Event event;

    while (game.isRunning()) {
        Uint32 frameStart = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            game.handleEvent(event);
        }

        game.update();
        SDL_RenderPresent(renderer);

        Uint32 elapsed = SDL_GetTicks() - frameStart;
        if (elapsed < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - elapsed);
        }
    }

    SDL_DestroyTexture(sprite);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
