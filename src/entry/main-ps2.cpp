#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <ps2_all_drivers.h>
#include <audsrv.h>
#include <cstdlib>
#include <cstdio>
#include <fstream>

#define DINO_RAND_FUNC_NAME mixIOPTimingSeed
#define DINO_RAND_FUNC_USES_CLASS

#include "defs.h"
#include "game.h"

#ifdef AUTOPLAYER
#include "auto_player.h"
#endif

#include "util.h"
#include "drawSprite.h"
#include "ps2_assets.h"
#include "../ps2/sdl_gamepad.h"

int   WINDOW_WIDTH  = 640;
int   WINDOW_HEIGHT = 448;
float MS_PER_FRAME  = 1000.0f / FPS;

namespace {

SDL_Texture* loadTile(SDL_Renderer* renderer, const unsigned char* bytes, unsigned int len) {
    SDL_Surface* surf = IMG_Load_RW(SDL_RWFromConstMem(bytes, (int)len), 1);
    if (!surf) {
        std::fprintf(stderr, "Sprite tile decode error: %s\n", IMG_GetError());
        return nullptr;
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    } else {
        std::fprintf(stderr, "Sprite tile texture error: %s\n", SDL_GetError());
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
#ifdef __PCSX2__
    freopen("DINOGAME-LOG-ERROR.TXT", "w", stderr);
    freopen("DINOGAME-LOG-OUTPUT.TXT", "w", stdout);
#endif
    init_joystick_driver(true);
    init_audio_driver();

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) { [[unlikely]]
        std::fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        deinit_audio_driver();
        deinit_joystick_driver(false);
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) { [[unlikely]]
        std::fprintf(stderr, "SDL_image init error: %s", IMG_GetError());
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

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) { [[unlikely]]
        std::fprintf(stderr, "Audio init error (continuing without audio): %s\n", Mix_GetError());
    } else {
        Mix_Init(MIX_INIT_MP3);
    }

    SDL_Window* window = SDL_CreateWindow(
        "DINOGAME",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!window) { [[unlikely]]
        std::fprintf(stderr, "Window creation error: %s\n", SDL_GetError());
        Mix_CloseAudio(); Mix_Quit(); IMG_Quit(); SDL_Quit();
        deinit_audio_driver();
        deinit_joystick_driver(false);
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) { [[unlikely]]
        std::fprintf(stderr, "Renderer creation error: %s\n", SDL_GetError());
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

    if (!tile1 || !tile2 || !tile1Inv || !tile2Inv) { [[unlikely]]
        std::fputs(stderr, "Failed to load one or more sprites, aborting...");
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
        Mix_CloseAudio(); Mix_Quit(); IMG_Quit(); SDL_Quit();
        deinit_audio_driver();
        deinit_joystick_driver(false);
        return 1;
    }

    ps2RegisterTiles(tile1, tile2, PS2_SPRITE_TILE_SPLIT);
    ps2RegisterTiles(tile1Inv, tile2Inv, PS2_SPRITE_TILE_SPLIT);

    ps2sdl::Ps2SdlPad pad;

    if (!pad.init()) { [[unlikely]]
        std::fprintf(stderr, "PS2 pad to SDL2 initialization failed: %s\n", SDL_GetError());
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

    Game game(renderer, tile1, tile1Inv, GET_RAND_SEED(pad));

#ifdef AUTOPLAYER
    AutoPlayer bot;
    bot.enabled = true;
#endif

    SDL_Event event;
    while (game.isRunning()) {
#ifdef AUTOPLAYER
        bot.tick(*game.getTrex(), *game.getHorizon(), game.getCurrentSpeed());
#endif
        pad.update();

        while (SDL_PollEvent(&event)) {
#ifdef AUTOPLAYER
            if (event.type == SDL_CONTROLLERBUTTONDOWN &&
                event.cbutton.button == ps2sdl::PS2SDL_CONTROLLER_BUTTON_SELECT)
            {
                bot.enabled = !bot.enabled;
            } else
#endif
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