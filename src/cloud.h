#pragma once
#include "defs.h"

class Cloud {
public:
    static constexpr int  WIDTH     = 46;
    static constexpr int  HEIGHT    = 14;
    static constexpr int  MIN_SKY   = 30;
    static constexpr int  MAX_SKY   = 71;
    static constexpr int  MIN_GAP   = 100;
    static constexpr int  MAX_GAP   = 400;

    float xPos  = 0.0f;
    float yPos  = 0.0f;
    float gap   = 0.0f;
    bool  remove = false;

    Cloud(SDL_Renderer* r, SDL_Texture* t, SDL_Texture* ti)
        : renderer_(r), sprite_(t), spriteInv_(ti)
    {
        xPos = (float)GAME_WIDTH;
        yPos = (float)randInt(MIN_SKY, MAX_SKY);
        gap  = (float)randInt(MIN_GAP, MAX_GAP);
        draw(false);
    }

    void update(float speed, bool night) {
        if (remove) return;
        xPos -= speed;
        if (xPos + WIDTH < 0) remove = true;
        draw(night);
    }

    void draw(bool night) const {
        drawSprite(renderer_, night ? spriteInv_ : sprite_,
                   SP_CLOUD.x, SP_CLOUD.y, WIDTH, HEIGHT,
                   (int)xPos, (int)yPos);
    }

private:
    SDL_Renderer* renderer_;
    SDL_Texture*  sprite_;
    SDL_Texture*  spriteInv_;
};
