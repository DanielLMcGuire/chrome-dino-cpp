#include "trex.h"
#include <cmath>

const Trex::FrameInfo Trex::ANIM_FRAMES[] = {
    /* WAITING  */ {{ 44,   0}, 1000.0f / 3.0f},
    /* RUNNING  */ {{ 88, 132}, 1000.0f / 12.0f},
    /* JUMPING  */ {{  0     }, 1000.0f / 60.0f},
    /* DUCKING  */ {{264, 323}, 1000.0f / 8.0f},
    /* CRASHED  */ {{220     }, 1000.0f / 60.0f},
};

static const std::vector<CollisionBox> COLL_RUNNING = {
    {22,  0, 17, 16},
    { 1, 18, 30,  9},
    {10, 35, 14,  8},
    { 1, 24, 29,  5},
    { 5, 30, 21,  4},
    { 9, 34, 15,  4},
};
static const std::vector<CollisionBox> COLL_DUCKING = {
    {1, 18, 55, 25},
};

Trex::Trex(SDL_Renderer* renderer, SDL_Texture* sprite, SDL_Texture* spriteInv)
    : renderer_(renderer), sprite_(sprite), spriteInv_(spriteInv)
{
    setBlinkDelay();
    animStartTime_ = SDL_GetTicks();
    update(0.0f, TrexStatus::WAITING);
}

void Trex::setBlinkDelay() {
    blinkDelay_ = 500.0f + randFloat() * 7000.0f;
}

void Trex::update(float deltaTime, TrexStatus newStatus, bool night) {
    animTimer_ += deltaTime;

    if (newStatus != TrexStatus(-1) && newStatus != status) {
        status = newStatus;
        currentFrame_  = 0;
        frameTimer_    = 0.0f;
    }

    if (playingIntro && xPos < TREX_START_X) {
        xPos += std::round((TREX_START_X / 1500.0f) * deltaTime);
    }

    if (status == TrexStatus::WAITING) {
        blink(SDL_GetTicks(), night);
    } else {
        const auto& fi = ANIM_FRAMES[(int)status];
        drawFrame(fi.frames[currentFrame_], 0, night);
    }

    frameTimer_ += deltaTime;
    const auto& fi = ANIM_FRAMES[(int)status];
    if (frameTimer_ >= fi.msPerFrame) {
        frameTimer_ = 0.0f;
        currentFrame_ = (currentFrame_ + 1) % (int)fi.frames.size();
    }

    if (speedDrop && (int)yPos == groundYPos_) {
        speedDrop = false;
        setDuck(true);
    }
}

void Trex::drawFrame(int xOffset, int /*yOffset*/, bool night) const {
    SDL_Texture* tex = night ? spriteInv_ : sprite_;
    int srcW = (ducking && status != TrexStatus::CRASHED)
                   ? TREX_WIDTH_DUCK : TREX_WIDTH;
    int srcH = TREX_HEIGHT;
    int srcX = SP_TREX.x + xOffset;
    int srcY = SP_TREX.y;

    int dstX = (int)xPos;
    int dstY = (int)yPos;
    int dstW = (ducking && status != TrexStatus::CRASHED) ? TREX_WIDTH_DUCK : TREX_WIDTH;
    int dstH = srcH;

    drawSprite(renderer_, tex, srcX, srcY, srcW, srcH, dstX, dstY, dstW, dstH);
}

void Trex::draw(bool night) const {
    const auto& fi = ANIM_FRAMES[(int)status];
    drawFrame(fi.frames[currentFrame_], 0, night);
}

void Trex::blink(Uint32 now, bool night) {
    float elapsed = (float)(now - animStartTime_);
    if (elapsed >= blinkDelay_) {
        const auto& fi = ANIM_FRAMES[(int)TrexStatus::WAITING];
        drawFrame(fi.frames[currentFrame_], 0, night);
        if (currentFrame_ == 1) {
            setBlinkDelay();
            animStartTime_ = now;
            ++blinkCount;
        }
    }
}

void Trex::startJump(float speed) {
    if (!jumping) {
        update(0.0f, TrexStatus::JUMPING);
        jumpVelocity_     = TREX_INITIAL_JUMP_VEL - (speed / 10.0f);
        jumping           = true;
        reachedMinHeight_ = false;
        speedDrop         = false;
    }
}

void Trex::endJump() {
    if (reachedMinHeight_ && jumpVelocity_ < TREX_DROP_VEL) {
        jumpVelocity_ = TREX_DROP_VEL;
    }
}

void Trex::updateJump(float deltaTime) {
    float msPerFrame = ANIM_FRAMES[(int)status].msPerFrame;
    float frames     = deltaTime / msPerFrame;

    if (speedDrop) {
        yPos += std::round(jumpVelocity_ * TREX_SPEED_DROP_COEFF * frames);
    } else {
        yPos += std::round(jumpVelocity_ * frames);
    }

    jumpVelocity_ += TREX_GRAVITY * frames;

    if (yPos < (float)minJumpHeight_ || speedDrop) {
        reachedMinHeight_ = true;
    }

    if (yPos < (float)TREX_MAX_JUMP_HEIGHT || speedDrop) {
        endJump();
    }

    if (yPos > (float)groundYPos_) {
        reset();
        ++jumpCount;
    }
}

void Trex::setSpeedDrop() {
    speedDrop     = true;
    jumpVelocity_ = 1.0f;
}

void Trex::setDuck(bool isDucking) {
    if (isDucking && status != TrexStatus::DUCKING) {
        update(0.0f, TrexStatus::DUCKING);
        ducking = true;
    } else if (status == TrexStatus::DUCKING) {
        update(0.0f, TrexStatus::RUNNING);
        ducking = false;
    }
}

void Trex::reset() {
    xPos          = (float)TREX_START_X;
    yPos          = (float)groundYPos_;
    jumpVelocity_ = 0.0f;
    jumping       = false;
    ducking       = false;
    update(0.0f, TrexStatus::RUNNING);
    speedDrop     = false;
    jumpCount     = 0;
}

std::vector<CollisionBox> Trex::getCollisionBoxes() const {
    return ducking ? COLL_DUCKING : COLL_RUNNING;
}
