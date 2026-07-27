#pragma once
#include <SDL2/SDL.h>
#include "defs.h"

inline void drawSprite(SDL_Renderer* r, SDL_Texture* t,
                       int sx, int sy, int sw, int sh,
                       int dx, int dy,
                       int dw = -1, int dh = -1,
                       double angle = 0.0,
                       SDL_RendererFlip flip = SDL_FLIP_NONE)
{
    SDL_Rect src = { sx, sy, sw, sh };
    SDL_Rect dst = {
        dx,
        dy,
        (dw < 0 ? sw / 1 : dw),
        (dh < 0 ? sh / 1 : dh)
    };
    if (angle != 0.0 || flip != SDL_FLIP_NONE)
        SDL_RenderCopyEx(r, t, &src, &dst, angle, nullptr, flip);
    else
        SDL_RenderCopy(r, t, &src, &dst);
}