#pragma once
#include "defs.h"

class HorizonLine {
public:
    static constexpr int SRC_X0     = 2;    // flat segment
    static constexpr int SRC_X1     = 602;  // bumpy segment
    static constexpr int SRC_Y      = 52;
    static constexpr int SEG_WIDTH  = 600;
    static constexpr int HEIGHT     = 12;
    static constexpr int Y_POS      = 127;  // logical y

    HorizonLine(SDL_Renderer* r, SDL_Texture* t, SDL_Texture* ti)
        : renderer_(r), sprite_(t), spriteInv_(ti)
    {
        xPos_[0] = 0;
        xPos_[1] = SEG_WIDTH;
        srcX_[0] = SRC_X0;
        srcX_[1] = SRC_X0;
        draw(false);
    }

    void update(float deltaTime, float speed, bool night) {
        float delta = std::floor(speed * FPS / 1000.0f * deltaTime);
        xPos_[0] -= delta;
        xPos_[1] -= delta;

        if (xPos_[0] + SEG_WIDTH <= 0) {
            xPos_[0] = xPos_[1] + SEG_WIDTH;
            srcX_[0] = (randFloat() > 0.5f) ? SRC_X1 : SRC_X0;
        }
        if (xPos_[1] + SEG_WIDTH <= 0) {
            xPos_[1] = xPos_[0] + SEG_WIDTH;
            srcX_[1] = (randFloat() > 0.5f) ? SRC_X1 : SRC_X0;
        }
        draw(night);
    }

    void reset() {
        xPos_[0] = 0;
        xPos_[1] = SEG_WIDTH;
        srcX_[0] = srcX_[1] = SRC_X0;
        draw(false);
    }

    void draw(bool night) const {
        SDL_Texture* tex = night ? spriteInv_ : sprite_;
        for (int i = 0; i < 2; ++i) {
            drawSprite(renderer_, tex,
                       srcX_[i], SRC_Y, SEG_WIDTH, HEIGHT,
                       (int)xPos_[i], Y_POS, SEG_WIDTH, HEIGHT);
        }
    }

private:
    SDL_Renderer* renderer_;
    SDL_Texture*  sprite_;
    SDL_Texture*  spriteInv_;
    float xPos_[2] = {};
    int   srcX_[2] = {};
};
