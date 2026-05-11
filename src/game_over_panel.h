#pragma once
#include "defs.h"

class GameOverPanel {
public:
    static constexpr int TEXT_W    = 191;
    static constexpr int TEXT_H    = 11;
    static constexpr int TEXT_SRC_X_OFFSET = 0;
    static constexpr int TEXT_SRC_Y_OFFSET = 13;
    static constexpr int RESTART_W = 36;
    static constexpr int RESTART_H = 32;
    static constexpr int RESTART_ANIM_DURATION = 875;
    static constexpr int NUM_FRAMES = 8;

    GameOverPanel(SDL_Renderer* r, SDL_Texture* t, SDL_Texture* ti)
        : renderer_(r), sprite_(t), spriteInv_(ti)
    {
        currentFrame_   = NUM_FRAMES - 1;
    }

    void draw(bool night) const {
        SDL_Texture* tex = night ? spriteInv_ : sprite_;
        int textX = (GAME_WIDTH  - TEXT_W) / 2;
        int textY = 45;
        drawSprite(renderer_, tex,
                   SP_TEXT.x + TEXT_SRC_X_OFFSET,
                   SP_TEXT.y + TEXT_SRC_Y_OFFSET,
                   TEXT_W, TEXT_H,
                   textX, textY);

        int restartX = (GAME_WIDTH  - RESTART_W) / 2;
        int restartY = 63;
        int frameOffset = currentFrame_ * RESTART_W;
        drawSprite(renderer_, tex,
                   SP_RESTART.x + frameOffset, SP_RESTART.y,
                   RESTART_W, RESTART_H,
                   restartX, restartY);
    }

    void reset() { currentFrame_ = NUM_FRAMES - 1; }

private:
    SDL_Renderer* renderer_;
    SDL_Texture*  sprite_;
    SDL_Texture*  spriteInv_;
    int currentFrame_ = 0;
};
