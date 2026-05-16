#pragma once
#include "defs.h"
#include "rand.h"

class NightMode {
public:
    static constexpr float FADE_SPEED = 0.035f;
    static constexpr int   WIDTH      = 20;
    static constexpr int   HEIGHT     = 40;
    static constexpr float MOON_SPEED = 0.25f;
    static constexpr int   NUM_STARS  = 2;
    static constexpr int   STAR_SIZE  = 9;
    static constexpr float STAR_SPEED = 0.3f;
    static constexpr int   STAR_MAX_Y = 70;

    static constexpr int PHASES[7] = {140, 120, 100, 60, 40, 20, 0};

    NightMode(SDL_Renderer* r, SDL_Texture* t, SDL_Texture* ti)
        : renderer_(r), sprite_(t), spriteInv_(ti)
    {
        placeStars();
    }

    void update(bool activated, bool night) {
        if (activated && opacity_ == 0.0f) {
            currentPhase_ = (currentPhase_ + 1) % 7;
        }

        if (activated && (opacity_ < 1.0f || opacity_ == 0.0f)) {
            opacity_ = std::min(1.0f, opacity_ + FADE_SPEED);
        } else if (opacity_ > 0.0f) {
            opacity_ = std::max(0.0f, opacity_ - FADE_SPEED);
        }

        if (opacity_ > 0.0f) {
            xPos_ = updateX(xPos_, MOON_SPEED);
            if (drawStars_) {
                for (auto& s : stars_) {
                    s.x = updateX(s.x, STAR_SPEED);
                }
            }
            draw(night);
        } else {
            opacity_ = 0.0f;
            placeStars();
        }
        drawStars_ = true;
    }

    void reset() {
        currentPhase_ = 0;
        opacity_      = 0.0f;
        update(false, false);
    }

private:
    SDL_Renderer* renderer_;
    SDL_Texture*  sprite_;
    SDL_Texture*  spriteInv_;

    float xPos_         = 0.0f;
    float yPos_         = 30.0f;
    int   currentPhase_ = 0;
    float opacity_      = 0.0f;
    bool  drawStars_    = false;

    struct Star { float x, y; int srcY; };
    Star stars_[NUM_STARS] = {};

    float updateX(float cur, float speed) {
        if (cur < -(float)WIDTH) return (float)GAME_WIDTH;
        return cur - speed;
    }

    void placeStars() {
        int seg = GAME_WIDTH / NUM_STARS;
        for (int i = 0; i < NUM_STARS; ++i) {
            stars_[i].x    = (float)randInt(seg * i, seg * (i + 1));
            stars_[i].y    = (float)randInt(0, STAR_MAX_Y);
            stars_[i].srcY = SP_STAR.y + STAR_SIZE * i;
        }
    }

    void draw(bool night) const {
        SDL_Texture* tex = night ? spriteInv_ : sprite_;
        Uint8 alpha = (Uint8)(opacity_ * 255.0f);
        SDL_SetTextureAlphaMod(tex, alpha);

        if (drawStars_) {
            for (const auto& s : stars_) {
                drawSprite(renderer_, tex,
                           SP_STAR.x, s.srcY, STAR_SIZE, STAR_SIZE,
                           (int)std::round(s.x), (int)s.y);
            }
        }

        int phase     = PHASES[currentPhase_];
        int moonW     = (currentPhase_ == 3) ? WIDTH * 2 : WIDTH;
        int srcMoonX  = SP_MOON.x + phase;
        drawSprite(renderer_, tex,
                   srcMoonX, SP_MOON.y, moonW, HEIGHT,
                   (int)std::round(xPos_), (int)yPos_);

        SDL_SetTextureAlphaMod(tex, 255);
    }
};

constexpr int NightMode::PHASES[7];
