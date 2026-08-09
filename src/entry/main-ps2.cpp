#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <ps2_all_drivers.h>
#include <audsrv.h>
#include <cstdlib>
#include <ctime>
#include <iostream>

#include "defs.h"
#include "game.h"

#ifdef AUTOPLAYER
#include "auto_player.h"
#endif

#include "util.h"
#include "drawSprite.h"
#include "ps2_assets.h"
#include "../ps2/sdl_joypad.h"

int   WINDOW_WIDTH  = 640;
int   WINDOW_HEIGHT = 448;
float MS_PER_FRAME  = 1000.0f / FPS;

namespace {

SDL_Texture* loadTile(SDL_Renderer* renderer, const unsigned char* bytes, unsigned int len) {
    SDL_Surface* surf = IMG_Load_RW(SDL_RWFromConstMem(bytes, (int)len), 1);
    if (!surf) {
        std::cerr << "Sprite tile decode error: " << IMG_GetError() << "\n";
        return nullptr;
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    } else {
        std::cerr << "Sprite tile texture error: " << SDL_GetError() << "\n";
    }
    SDL_FreeSurface(surf);
    return tex;
}

SDL_Texture* loadInvertedTile(SDL_Renderer* renderer, const unsigned char* bytes, unsigned int len) {
    SDL_Surface* surf = IMG_Load_RW(SDL_RWFromConstMem(bytes, (int)len), 1);
    if (!surf) return nullptr;
    SDL_Surface* inv = createInvertedSurface(surf);
    SDL_FreeSurface(surf);
    if (!inv) return nullptr;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, inv);
    if (tex) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    }
    SDL_FreeSurface(inv);
    return tex;
}

} // namespace

int main(int /*argc*/, char* /*argv*/[]) {
    std::srand(static_cast<unsigned>(std::time(nullptr))); // NOLINT

    init_joystick_driver(true);
    init_audio_driver();

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "SDL_Init error: " << SDL_GetError() << "\n";
        deinit_audio_driver();
        deinit_joystick_driver(false);
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "SDL_image init error: " << IMG_GetError() << "\n";
        SDL_Quit();
        deinit_audio_driver();
        deinit_joystick_driver(false);
        return 1;
    }

    if (audsrv_init() == 0) {
        audsrv_fmt_t fmt{};
        fmt.freq = 44100;
        fmt.bits = 16;
        fmt.channels = 2;
        audsrv_set_format(&fmt);
        audsrv_set_volume(MAX_VOLUME);
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Audio init error (continuing without audio): " << Mix_GetError() << "\n";
    } else {
        Mix_Init(MIX_INIT_MP3);
    }

    SDL_Window* window = SDL_CreateWindow(
        "Chromium Dino Game",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        std::cerr << "Window creation error: " << SDL_GetError() << "\n";
        Mix_CloseAudio(); Mix_Quit(); IMG_Quit(); SDL_Quit();
        deinit_audio_driver();
        deinit_joystick_driver(false);
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        std::cerr << "Renderer creation error: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        Mix_CloseAudio(); Mix_Quit(); IMG_Quit(); SDL_Quit();
        deinit_audio_driver();
        deinit_joystick_driver(false);
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, GAME_WIDTH, GAME_HEIGHT);

    SDL_Texture* tile1    = loadTile(renderer, SPRITE_SHEET_TILE1, SPRITE_SHEET_TILE1_len);
    SDL_Texture* tile2    = loadTile(renderer, SPRITE_SHEET_TILE2, SPRITE_SHEET_TILE2_len);
    SDL_Texture* tile1Inv = loadInvertedTile(renderer, SPRITE_SHEET_TILE1, SPRITE_SHEET_TILE1_len);
    SDL_Texture* tile2Inv = loadInvertedTile(renderer, SPRITE_SHEET_TILE2, SPRITE_SHEET_TILE2_len);

    if (!tile1 || !tile2 || !tile1Inv || !tile2Inv) {
        std::cerr << "Failed to load one or more sprite tiles -- aborting.\n";
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
        Mix_CloseAudio(); Mix_Quit(); IMG_Quit(); SDL_Quit();
        deinit_audio_driver();
        deinit_joystick_driver(false);
        return 1;
    }

    ps2RegisterTiles(tile1, tile2, PS2_SPRITE_TILE_SPLIT);
    ps2RegisterTiles(tile1Inv, tile2Inv, PS2_SPRITE_TILE_SPLIT);

    Game game(renderer, tile1, tile1Inv);

#ifdef AUTOPLAYER
    AutoPlayer bot;
    bot.enabled = true;
#endif

    ps2sdl::Ps2SdlPad pad;
    if (!pad.init()) {
        std::cerr << "PS2 pad to SDL2 initialization failed: " << SDL_GetError() << "\n";
        SDL_DestroyTexture(tile1);
        SDL_DestroyTexture(tile2);
        SDL_DestroyTexture(tile1Inv);
        SDL_DestroyTexture(tile2Inv);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        deinit_audio_driver();
        deinit_joystick_driver(false);
        return 1;
    }

    SDL_Event event;
    while (game.isRunning()) {
#ifdef AUTOPLAYER
        bot.tick(*game.getTrex(), *game.getHorizon(), game.getCurrentSpeed());
#endif
        pad.update();

        while (SDL_PollEvent(&event)) {
            game.handleEvent(event);
        }

        game.update();
        SDL_RenderPresent(renderer);
    }

    pad.shutdown();

    SDL_DestroyTexture(tile1);
    SDL_DestroyTexture(tile2);
    SDL_DestroyTexture(tile1Inv);
    SDL_DestroyTexture(tile2Inv);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    deinit_audio_driver();
    deinit_joystick_driver(false);
    return 0;
}