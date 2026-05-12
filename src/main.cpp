#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <iostream>
#include <cstdlib>
#include <ctime>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#include "defs.h"
#include "game.h"

int   WINDOW_WIDTH  = GAME_WIDTH  * 2;
int   WINDOW_HEIGHT = GAME_HEIGHT * 2;
float MS_PER_FRAME  = 1000.0f / FPS;

int SDL_main(int /*argc*/, char* /*argv*/[]) {
    std::srand((unsigned)std::time(nullptr));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
#ifdef _WIN32
        MessageBoxA(NULL, "SDL initialization error", "DinoGame | Error", MB_OK | MB_ICONERROR);
#else
        std::cerr << "SDL_Init error: " << SDL_GetError() << "\n";
#endif
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DinoGame | Error",
            (std::string("SDL image initialization error: ") + IMG_GetError()).c_str(), nullptr);
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "DinoGame | Warning",
            (std::string("Audio loading error: ") + Mix_GetError() + "\nContinuing without audio.").c_str(), nullptr);
        IMG_Quit();
        SDL_Quit();
    }

    Mix_Init(MIX_INIT_MP3);

    SDL_Window* window = SDL_CreateWindow(
        "Chromium Dino Game",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!window) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DinoGame | Error",
            (std::string("Window creation error: ") + SDL_GetError()).c_str(), nullptr);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DinoGame | Error",
            (std::string("Renderer creation error: ") + SDL_GetError()).c_str(), window);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    int displayIndex = SDL_GetWindowDisplayIndex(window);
    SDL_DisplayMode mode;
    if (SDL_GetCurrentDisplayMode(displayIndex, &mode) == 0 && mode.refresh_rate > 0) {
        MS_PER_FRAME = 1000.0f / (float)mode.refresh_rate;
    }

    int drawW = 0, drawH = 0;
    SDL_GetRendererOutputSize(renderer, &drawW, &drawH);
    IS_HIDPI = (drawW > WINDOW_WIDTH);

    SDL_RenderSetLogicalSize(renderer, GAME_WIDTH, GAME_HEIGHT);

    const char* spritePath = IS_HIDPI
        ? "resources/images/default_200_percent/offline/200-offline-sprite.png"
        : "resources/images/default_100_percent/offline/100-offline-sprite.png";

    SDL_Surface* surf = IMG_Load(spritePath);
    if (!surf) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DinoGame | Error",
            (std::string("Image loading error: ") + IMG_GetError()).c_str(), window);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* sprite = SDL_CreateTextureFromSurface(renderer, surf);
    if (!sprite) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DinoGame | Error",
            (std::string("Texture creation error: ") + SDL_GetError()).c_str(), window);
        SDL_FreeSurface(surf);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_SetTextureBlendMode(sprite, SDL_BLENDMODE_BLEND);

    SDL_Surface* surfInv = createInvertedSurface(surf);
    SDL_FreeSurface(surf);
    SDL_Texture* spriteInv = SDL_CreateTextureFromSurface(renderer, surfInv);
    SDL_FreeSurface(surfInv);
    if (!spriteInv) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DinoGame | Error",
            (std::string("Texture creation (inv) error: ") + SDL_GetError()).c_str(), window);
        SDL_DestroyTexture(sprite);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_SetTextureBlendMode(spriteInv, SDL_BLENDMODE_BLEND);

    Game game(renderer, sprite, spriteInv);

    SDL_Event event;

    while (game.isRunning()) {
        while (SDL_PollEvent(&event)) {
            game.handleEvent(event);
        }

        game.update();
        SDL_RenderPresent(renderer);  // blocks at display refresh rate via vsync
    }

    SDL_DestroyTexture(spriteInv);
    SDL_DestroyTexture(sprite);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
