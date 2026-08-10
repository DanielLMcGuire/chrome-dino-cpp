#pragma once
#include "defs.h"
#include <cstdint>
#include <ctime>
#include <SDL2/SDL.h>

const ObstacleTypeDef& getCactusSmallDef();
const ObstacleTypeDef& getCactusLargeDef();
const ObstacleTypeDef& getPterodactylDef();

inline SDL_Surface* createInvertedSurface(SDL_Surface* src) {
    SDL_Surface* conv = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA32, 0);
    if (!conv) return nullptr;
    SDL_LockSurface(conv);
    auto* px = (Uint8*)conv->pixels;
    for (int i = 0; i < conv->h; ++i) {
        for (int j = 0; j < conv->w; ++j) {
            Uint8* p = px + i * conv->pitch + j * 4;
            p[0] = 255 - p[0]; // R
            p[1] = 255 - p[1]; // G
            p[2] = 255 - p[2]; // B
        }
    }
    SDL_UnlockSurface(conv);
    return conv;
}

using ColorRGBA = std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>;

#ifdef UWP
std::vector<uint8_t> LoadFileBytes(const char* uri);
#endif

static constexpr ColorRGBA HexToRGBA(uint32_t hex)
{
    const bool hasAlpha = hex > 0xFFFFFF;

    const uint8_t r = hasAlpha ? (hex >> 24) & 0xFF : (hex >> 16) & 0xFF;
    const uint8_t g = hasAlpha ? (hex >> 16) & 0xFF : (hex >> 8)  & 0xFF;
    const uint8_t b = hasAlpha ? (hex >> 8)  & 0xFF :  hex        & 0xFF;
    const uint8_t a = hasAlpha ?  hex        & 0xFF : 255;

    return { r, g, b, a };
}

static std::uint32_t getSeed() {
    const std::uint32_t timeSeed =
    static_cast<std::uint32_t>(std::time(nullptr));

    const std::uint32_t timerSeed =
        static_cast<std::uint32_t>(SDL_GetTicks());

    const std::uint32_t addressSeed =
        static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(&timeSeed)
        );

    static std::uint32_t seed = timeSeed;

    seed ^= timerSeed + 0x9e3779b9u + (seed << 6) + (seed >> 2);
    seed ^= addressSeed + 0x9e3779b9u + (seed << 6) + (seed >> 2);

    seed ^= seed >> 16;
    seed *= 0x85ebca6bu;
    seed ^= seed >> 13;
    seed *= 0xc2b2ae35u;
    seed ^= seed >> 16;

    return seed;
}