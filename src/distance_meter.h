#pragma once
#include "defs.h"
#include <string>
#include <vector>

class DistanceMeter {
public:
    static constexpr int   CHAR_WIDTH       = 10;
    static constexpr int   CHAR_HEIGHT      = 13;
    static constexpr int   DEST_WIDTH       = 11;
    static constexpr int   MAX_UNITS        = 5;
    static constexpr int   ACHIEVEMENT_DIST = 100;
    static constexpr float COEFFICIENT      = 0.025f;
    static constexpr float FLASH_DURATION   = 1000.0f / 4.0f;
    static constexpr int   FLASH_ITERATIONS = 3;

    bool achievement = false;

    DistanceMeter(SDL_Renderer* r, SDL_Texture* t, SDL_Texture* ti)
        : renderer_(r), sprite_(t), spriteInv_(ti)
    {
        maxUnits_ = MAX_UNITS;
        maxScore_ = maxUnits_;
        calcX();
        defaultString_.assign(maxUnits_, '0');
        for (int i = 0; i < maxUnits_; ++i) {
            maxScoreStr_ += "9";
        }
        maxScore_ = std::stoi(maxScoreStr_);
    }

    void reset() {
        achievement     = false;
        flashTimer_     = 0.0f;
        flashIterations_= 0;
        digits_.clear();
        for (int i = 0; i < maxUnits_; ++i) digits_.push_back('0');
    }

    void setHighScore(int distance) {
        int d = getActualDistance(distance);
        std::string s = std::string(maxUnits_, '0') + std::to_string(d);
        highScore_ = "HI " + s.substr(s.size() - maxUnits_);
    }

    int getActualDistance(float distance) const {
        return distance > 0 ? (int)std::round(distance * COEFFICIENT) : 0;
    }

    bool update(float deltaTime, int distance, bool night) {
        bool playSound = false;
        bool paint     = true;

        if (!achievement) {
            int d = getActualDistance((float)distance);

            if (d > maxScore_ && maxUnits_ == MAX_UNITS) {
                ++maxUnits_;
                maxScoreStr_ += "9";
                maxScore_     = std::stoi(maxScoreStr_);
            }

            if (d > 0) {
                if (d % ACHIEVEMENT_DIST == 0) {
                    achievement     = true;
                    flashTimer_     = 0.0f;
                    flashIterations_= 0;
                    playSound       = true;
                }
                std::string s = std::string(maxUnits_, '0') + std::to_string(d);
                std::string ds = s.substr(s.size() - maxUnits_);
                digits_.assign(ds.begin(), ds.end());
            } else {
                digits_.assign(maxUnits_, '0');
            }
        } else {
            if (flashIterations_ <= FLASH_ITERATIONS) {
                flashTimer_ += deltaTime;
                if (flashTimer_ < FLASH_DURATION) {
                    paint = false;
                } else if (flashTimer_ > FLASH_DURATION * 2.0f) {
                    flashTimer_ = 0.0f;
                    ++flashIterations_;
                }
            } else {
                achievement      = false;
                flashIterations_ = 0;
                flashTimer_      = 0.0f;
            }
        }

        if (paint) {
            for (int i = 0; i < (int)digits_.size(); ++i) {
                drawDigit(i, digits_[i] - '0', false, night);
            }
        }
        drawHighScore(night);
        return playSound;
    }

private:
    SDL_Renderer* renderer_;
    SDL_Texture*  sprite_;
    SDL_Texture*  spriteInv_;

    int   x_               = 0;
    int   y_               = 5;
    int   maxUnits_        = MAX_UNITS;
    int   maxScore_        = 0;
    std::string maxScoreStr_;
    std::string defaultString_;
    std::vector<char> digits_;
    std::string highScore_;
    float flashTimer_      = 0.0f;
    int   flashIterations_ = 0;

    void calcX() {
        x_ = GAME_WIDTH - DEST_WIDTH * (maxUnits_ + 1);
    }

    void drawDigit(int digitPos, int charPos, bool isHighScore, bool night) const {
        SDL_Texture* tex = night ? spriteInv_ : sprite_;
        int srcX = SP_TEXT.x + CHAR_WIDTH * charPos;
        int srcY = SP_TEXT.y;

        int baseX = isHighScore
                       ? x_ - maxUnits_ * 2 * CHAR_WIDTH
                       : x_;
        int dstX = baseX + digitPos * DEST_WIDTH;
        int dstY = y_;

        drawSprite(renderer_, tex,
                   srcX, srcY, CHAR_WIDTH, CHAR_HEIGHT,
                   dstX, dstY);
    }

    void drawHighScore(bool night) const {
        if (highScore_.empty()) return;
        SDL_Texture* tex = night ? spriteInv_ : sprite_;
        SDL_SetTextureAlphaMod(tex, (Uint8)(255 * 0.8f));
        for (int i = 0; i < (int)highScore_.size(); ++i) {
            char c = highScore_[i];
            int charPos = -1;
            if (c >= '0' && c <= '9') {
                charPos = c - '0';
            } else if (c == 'H') {
                charPos = 10;
            } else if (c == 'I') {
                charPos = 11;
            }
            if (charPos >= 0) {
                drawDigit(i, charPos, true, night);
            }
        }
        SDL_SetTextureAlphaMod(tex, 255);
    }
};
