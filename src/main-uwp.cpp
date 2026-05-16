#pragma comment(lib, "runtimeobject.lib")
#include <winrt/Windows.UI.Popups.h>
#include <winrt/Windows.Foundation.h>
#include <roapi.h>
#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <cstdlib>
#include <ctime>
#include <future>
#include <string>

#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>

#include "defs.h"
#include "game.h"

#ifdef AUTOPLAYER
#include "auto_player.h"
#endif

int   WINDOW_WIDTH  = GAME_WIDTH  * 2;
int   WINDOW_HEIGHT = GAME_HEIGHT * 2;
float MS_PER_FRAME  = 1000.0f / FPS;

static void ShowError(const wchar_t* title, const wchar_t* message)
{
    winrt::Windows::UI::Popups::MessageDialog dialog{ message, title };
    dialog.ShowAsync().get();
}

std::vector<uint8_t> LoadFileBytes(const char* uri)
{
    std::string uriStr(uri);
    return std::async(std::launch::async, [uriStr]() -> std::vector<uint8_t>
        {
            auto uri = winrt::Windows::Foundation::Uri(winrt::to_hstring(uriStr));
            auto file = winrt::Windows::Storage::StorageFile::GetFileFromApplicationUriAsync(uri).get();
            auto stream = file.OpenReadAsync().get();
            auto size = (uint32_t)stream.Size();
            winrt::Windows::Storage::Streams::DataReader reader(stream);
            reader.LoadAsync(size).get();
            std::vector<uint8_t> bytes(size);
            reader.ReadBytes(bytes);
            return bytes;
        }).get();
}

int main(int argc, char* argv[])
{
    std::srand((unsigned)std::time(nullptr));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        ShowError(L"DinoGame | Error", L"SDL initialization error");
        winrt::uninit_apartment();
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DinoGame | Error",
            (std::string("SDL image initialization error: ") + IMG_GetError()).c_str(), nullptr);
        SDL_Quit();
        winrt::uninit_apartment();
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
#ifdef AUTOPLAYER
        "Chromium Dino Game [AUTOPLAYER]",
#else
        "Chromium Dino Game",
#endif
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!window) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DinoGame | Error",
            (std::string("Window creation error: ") + SDL_GetError()).c_str(), nullptr);
        Mix_CloseAudio(); Mix_Quit(); IMG_Quit(); SDL_Quit();
        winrt::uninit_apartment();
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
        Mix_CloseAudio(); Mix_Quit(); IMG_Quit(); SDL_Quit();
        winrt::uninit_apartment();
        return 1;
    }

    int displayIndex = SDL_GetWindowDisplayIndex(window);
    SDL_DisplayMode mode;
    if (SDL_GetCurrentDisplayMode(displayIndex, &mode) == 0 && mode.refresh_rate > 0)
        MS_PER_FRAME = 1000.0f / (float)mode.refresh_rate;

    SDL_RenderSetLogicalSize(renderer, GAME_WIDTH, GAME_HEIGHT);

    const char* spritePath = "ms-appx:///resources/100-offline-sprite.png";

    auto bytes = LoadFileBytes(spritePath);
    SDL_RWops* rw = SDL_RWFromMem(bytes.data(), (int)bytes.size());
    SDL_Surface* surf = IMG_Load_RW(rw, 1);
    if (!surf) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DinoGame | Error",
            (std::string("Image loading error: ") + IMG_GetError()).c_str(), window);
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
        Mix_CloseAudio(); Mix_Quit(); IMG_Quit(); SDL_Quit();
        return 1;
    }

    SDL_Texture* sprite = SDL_CreateTextureFromSurface(renderer, surf);
    if (!sprite) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DinoGame | Error",
            (std::string("Texture creation error: ") + SDL_GetError()).c_str(), window);
        SDL_FreeSurface(surf);
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
        Mix_CloseAudio(); Mix_Quit(); IMG_Quit(); SDL_Quit();
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
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
        Mix_CloseAudio(); Mix_Quit(); IMG_Quit(); SDL_Quit();
        return 1;
    }
    SDL_SetTextureBlendMode(spriteInv, SDL_BLENDMODE_BLEND);

    Game game(renderer, sprite, spriteInv);

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
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_TAB)
                bot.enabled = !bot.enabled;
            else
#endif
            game.handleEvent(event);
        }

        game.update();
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(spriteInv);
    SDL_DestroyTexture(sprite);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio(); Mix_Quit(); IMG_Quit(); SDL_Quit();
    return 0;
}