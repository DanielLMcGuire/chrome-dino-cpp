#pragma once
#include "defs.h"
#include "rand.h"
#include "drawSprite.h"

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
        float increment = std::floor(speed * (FPS / 1000.0f) * deltaTime);
        int pos = xPos_[0] <= 0 ? 0 : 1;
        updatexPos(pos, increment);
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

    void updatexPos(int pos, float increment) {
        int line1 = pos;
        int line2 = pos == 0 ? 1 : 0;
        xPos_[line1] -= increment;
        xPos_[line2] = xPos_[line1] + SEG_WIDTH;
        if (xPos_[line1] <= -(float)SEG_WIDTH) {
            xPos_[line1] += SEG_WIDTH * 2;
            xPos_[line2]  = xPos_[line1] - SEG_WIDTH;
            srcX_[line1]  = getRandomType();
        }
    }

    int getRandomType() const {
        return randFloat() > 0.5f ? SRC_X0 + SEG_WIDTH : SRC_X0;
    }
};
