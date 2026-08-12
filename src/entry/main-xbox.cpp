#include <SDL.h>
#include <SDL_image.h>

extern "C" {
#include <hal/xbox.h>
#include <hal/led.h>
}

#include <windows.h>
#include <cstdlib>

#include "defs.h"
#include "game.h"
#include "util.h"
#include "xbox_assets.h"
#include "../xbox/tools.h"

#ifdef AUTOPLAYER
#include "auto_player.h"
#endif

int   WINDOW_WIDTH = 640;
int   WINDOW_HEIGHT = 480;
float MS_PER_FRAME  = 1000.0f / FPS;

namespace {

SDL_Texture* loadSpriteTexture(SDL_Renderer* renderer) {
    SDL_Surface* surf = IMG_Load_RW(SDL_RWFromConstMem(SPRITE_SHEET, (int)SPRITE_SHEET_len), 1);
    if (!surf) { [[unlikely]]
#ifdef _DEBUG
        XSetCustomLED(XLEDColor::XLED_ORANGE, XLEDColor::XLED_GREEN, XLEDColor::XLED_ORANGE, XLEDColor::XLED_GREEN);
#endif
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Failed to load sprite texture",
            SDL_GetError(),
            nullptr
        );
        Sleep(5000);
        xboxhelper::logError("Sprite decode error", IMG_GetError());
        return nullptr;
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    } else { [[unlikely]]
#ifdef _DEBUG
        XSetCustomLED(XLEDColor::XLED_ORANGE, XLEDColor::XLED_RED, XLEDColor::XLED_ORANGE, XLEDColor::XLED_RED);
#endif
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Failed to load sprite texture",
            SDL_GetError(),
            nullptr
        );
        Sleep(5000);
        xboxhelper::logError("Sprite texture error", SDL_GetError());
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
    xboxhelper::resolution_t res = xboxhelper::initXboxVideo();
    WINDOW_WIDTH = res.width;
    WINDOW_HEIGHT = res.height;
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) { [[unlikely]]
        xboxhelper::logError("SDL_Init error", SDL_GetError(), true);
        __builtin_unreachable();
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) { [[unlikely]]
        SDL_Quit();
        xboxhelper::logError("SDL_image init error", IMG_GetError(), true);
        __builtin_unreachable();
    }

    SDL_Window* window = SDL_CreateWindow(
        "DINOGAME",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!window) { [[unlikely]]
        IMG_Quit(); SDL_Quit();
        xboxhelper::logError("Window creation error", SDL_GetError(), true);
        __builtin_unreachable();
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) { [[unlikely]]
        SDL_DestroyWindow(window);
        IMG_Quit(); SDL_Quit();
        xboxhelper::logError("Renderer creation error", SDL_GetError(), true);
        __builtin_unreachable();
    }

    SDL_RenderSetLogicalSize(renderer, GAME_WIDTH, GAME_HEIGHT);

    SDL_Texture* sprite    = loadSpriteTexture(renderer);
    SDL_Texture* spriteInv = loadInvertedSpriteTexture(renderer);

    if (!sprite || !spriteInv) { [[unlikely]]
        XSetCustomLED(XLEDColor::XLED_GREEN, XLEDColor::XLED_RED, XLEDColor::XLED_RED, XLEDColor::XLED_ORANGE);
        if (sprite)    SDL_DestroyTexture(sprite);
        if (spriteInv) SDL_DestroyTexture(spriteInv);
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Failed to load sprite texture",
            SDL_GetError(),
            window
        );
        Sleep(5000);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit(); SDL_Quit();
        xboxhelper::logError("Failed to load sprite texture", "aborting", true);
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
