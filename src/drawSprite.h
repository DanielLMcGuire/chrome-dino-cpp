#pragma once
#ifdef __XBOX__
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include "defs.h"

#ifdef __PS2__
struct Ps2SpriteTiles {
    SDL_Texture* tile1;
    SDL_Texture* tile2;
    int splitX;
};

inline Ps2SpriteTiles g_ps2TileRegistry[4];
inline int g_ps2TileCount = 0;

inline void ps2RegisterTiles(SDL_Texture* tile1, SDL_Texture* tile2, int splitX) {
    if (g_ps2TileCount < 4) {
        g_ps2TileRegistry[g_ps2TileCount++] = {tile1, tile2, splitX};
    }
}
#endif

inline void drawSprite(SDL_Renderer* r, SDL_Texture* t,
                       int sx, int sy, int sw, int sh,
                       int dx, int dy,
                       int dw = -1, int dh = -1,
                       double angle = 0.0,
                       SDL_RendererFlip flip = SDL_FLIP_NONE)
{
    SDL_Texture* useTex = t;
    int useSx = sx;
#ifdef __PS2__
    for (int i = 0; i < g_ps2TileCount; i++) {
        if (g_ps2TileRegistry[i].tile1 == t) {
            if (sx >= g_ps2TileRegistry[i].splitX) {
                useTex = g_ps2TileRegistry[i].tile2;
                useSx  = sx - g_ps2TileRegistry[i].splitX;
            }
            break;
        }
    }
#endif
    SDL_Rect src = { useSx, sy, sw, sh };
    SDL_Rect dst = {
        dx,
        dy,
        (dw < 0 ? sw / 1 : dw),
        (dh < 0 ? sh / 1 : dh)
    };
    if (angle != 0.0 || flip != SDL_FLIP_NONE)
        SDL_RenderCopyEx(r, useTex, &src, &dst, angle, nullptr, flip);
    else
        SDL_RenderCopy(r, useTex, &src, &dst);
}