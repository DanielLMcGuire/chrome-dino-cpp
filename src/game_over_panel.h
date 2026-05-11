#pragma once
#include "defs.h"

class GameOverPanel {
public:
    static constexpr int   TEXT_W              = 191;
    static constexpr int   TEXT_H              = 11;
    static constexpr int   TEXT_SRC_X_OFFSET   = 0;
    static constexpr int   TEXT_SRC_Y_OFFSET   = 13;
    static constexpr int   RESTART_W           = 36;
    static constexpr int   RESTART_H           = 32;
    static constexpr int   NUM_FRAMES          = 8;
    static constexpr float LOGO_PAUSE_DURATION = 875.0f;
    static constexpr float MS_PER_FRAME        = 875.0f / NUM_FRAMES;

    // Frame x-offsets in sprite sheet (each frame is 36px wide)
    static constexpr int FRAMES[NUM_FRAMES] = {0, 36, 72, 108, 144, 180, 216, 252};

    GameOverPanel(SDL_Renderer* r, SDL_Texture* t, SDL_Texture* ti)
        : renderer_(r), sprite_(t), spriteInv_(ti) {}

    void update(float deltaTime, bool night) {
        animTimer_ += deltaTime;

        if (currentFrame_ == 0) {
            if (animTimer_ >= LOGO_PAUSE_DURATION) {
                animTimer_ = 0.0f;
                currentFrame_++;
            }
        } else if (currentFrame_ < NUM_FRAMES) {
            if (animTimer_ >= MS_PER_FRAME) {
                animTimer_ = 0.0f;
                currentFrame_++;
            }
        }

        draw(night);
    }

    void draw(bool night) const {
        SDL_Texture* tex = night ? spriteInv_ : sprite_;

        // "GAME OVER" text — centered, y = (height - 25) / 3
        int textX = (GAME_WIDTH - TEXT_W) / 2;
        int textY = (GAME_HEIGHT - 25) / 3;
        drawSprite(renderer_, tex,
                   SP_TEXT.x + TEXT_SRC_X_OFFSET,
                   SP_TEXT.y + TEXT_SRC_Y_OFFSET,
                   TEXT_W, TEXT_H,
                   textX, textY);

        // Restart button — centered using restartHeight for x, y = height / 2
        int restartX = (GAME_WIDTH / 2) - (RESTART_H / 2);
        int restartY = GAME_HEIGHT / 2;
        int frame    = currentFrame_ < NUM_FRAMES ? currentFrame_ : NUM_FRAMES - 1;
        drawSprite(renderer_, tex,
                   SP_RESTART.x + FRAMES[frame], SP_RESTART.y,
                   RESTART_W, RESTART_H,
                   restartX, restartY);
    }

    void reset() {
        currentFrame_ = 0;
        animTimer_    = 0.0f;
    }

private:
    SDL_Renderer* renderer_;
    SDL_Texture*  sprite_;
    SDL_Texture*  spriteInv_;

    int   currentFrame_ = 0;
    float animTimer_    = 0.0f;
};

constexpr int GameOverPanel::FRAMES[GameOverPanel::NUM_FRAMES];
