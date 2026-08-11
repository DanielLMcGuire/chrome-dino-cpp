#ifdef __XBOX__
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <SDL_image.h>

extern "C" {
#include <hal/debug.h>
#include <hal/video.h>
#include <hal/xbox.h>
#include <windows.h>
}

#include <cstdlib>
#include <cstdio>

#include "defs.h"
#include "game.h"
#include "util.h"
#include "xbox_assets.h"

#ifdef AUTOPLAYER
#include "auto_player.h"
#endif

int   WINDOW_WIDTH  = 640;
int   WINDOW_HEIGHT = 480;
float MS_PER_FRAME  = 1000.0f / FPS;

namespace {

void logError(const char* what, const char* detail) {
    debugPrint("%s: %s\n", what, detail);
#ifdef _DEBUG
    std::fputs(stderr, "%s: %s", what, detail);
#endif
}

SDL_Texture* loadSpriteTexture(SDL_Renderer* renderer) {
    SDL_Surface* surf = IMG_Load_RW(SDL_RWFromConstMem(SPRITE_SHEET, (int)SPRITE_SHEET_len), 1);
    if (!surf) { [[unlikely]]
        logError("Sprite decode error", IMG_GetError());
        return nullptr;
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    } else { [[unlikely]]
        logError("Sprite texture error", SDL_GetError());
    }
    SDL_FreeSurface(surf);
    return tex;
}

SDL_Texture* loadInvertedSpriteTexture(SDL_Renderer* renderer) {
    SDL_Surface* surf = IMG_Load_RW(SDL_RWFromConstMem(SPRITE_SHEET, (int)SPRITE_SHEET_len), 1);
    if (!surf) {[[unlikely]] return nullptr; }
    SDL_Surface* inv = createInvertedSurface(surf);
    SDL_FreeSurface(surf);
    if (!inv) { [[unlikely]] return nullptr; }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, inv);
    if (tex) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    }
    SDL_FreeSurface(inv);
    return tex;
}

} // namespace

void main() {
    XVideoSetMode(WINDOW_WIDTH, WINDOW_HEIGHT, 32, REFRESH_DEFAULT);
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) { [[unlikely]]
        logError("SDL_Init error", SDL_GetError());
        Sleep(5000);
        XReboot();
        __builtin_unreachable();
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) { [[unlikely]]
        logError("SDL_image init error", IMG_GetError());
        SDL_Quit();
        Sleep(5000);
        XReboot();
        __builtin_unreachable();
    }

    SDL_Window* window = SDL_CreateWindow(
        "DINOGAME",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!window) { [[unlikely]]
        logError("Window creation error", SDL_GetError());
        IMG_Quit(); SDL_Quit();
        Sleep(5000);
        XReboot();
        __builtin_unreachable();
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) { [[unlikely]]
        logError("Renderer creation error", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit(); SDL_Quit();
        Sleep(5000);
        XReboot();
        __builtin_unreachable();
    }

    SDL_RenderSetLogicalSize(renderer, GAME_WIDTH, GAME_HEIGHT);

    SDL_Texture* sprite    = loadSpriteTexture(renderer);
    SDL_Texture* spriteInv = loadInvertedSpriteTexture(renderer);

    if (!sprite || !spriteInv) { [[unlikely]]
        logError("Failed to load sprite texture", "aborting");
        if (sprite)    SDL_DestroyTexture(sprite);
        if (spriteInv) SDL_DestroyTexture(spriteInv);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit(); SDL_Quit();
        Sleep(5000);
        XReboot();
        __builtin_unreachable();
    }

    Game game(renderer, sprite, spriteInv, GET_RAND_SEED());

#ifdef AUTOPLAYER
    AutoPlayer bot;
    bot.enabled = true;
#endif

    SDL_Event event;
    while (game.isRunning()) {
#ifdef AUTOPLAYER
        bot.tick(*game.getTrex(), *game.getHorizon(), game.getCurrentSpeed());
#endif
        while (SDL_PollEvent(&event)) {
#ifdef AUTOPLAYER
            if (event.type == SDL_CONTROLLERBUTTONDOWN &&
                event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK)
            { [[unlikely]]
                bot.enabled = !bot.enabled;
            } else
#endif
            game.handleEvent(event);
        }

        game.update();
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(sprite);
    SDL_DestroyTexture(spriteInv);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    XReboot();
    __builtin_unreachable();
}
