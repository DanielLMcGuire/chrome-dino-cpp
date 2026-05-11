#pragma once
#include "defs.h"

enum class TrexStatus { WAITING, RUNNING, JUMPING, DUCKING, CRASHED };

class Trex {
public:
    float xPos    = TREX_START_X;
    float yPos    = (float)GROUND_Y;
    bool  jumping = false;
    bool  ducking = false;
    bool  speedDrop  = false;
    int   jumpCount  = 0;
    int   blinkCount = 0;
    bool  playingIntro = false;

    TrexStatus status = TrexStatus::WAITING;

    Trex(SDL_Renderer* renderer, SDL_Texture* sprite);

    void update(float deltaTime, TrexStatus newStatus = TrexStatus(-1));
    void updateJump(float deltaTime);
    void draw() const;

    void startJump(float speed);
    void endJump();
    void setSpeedDrop();
    void setDuck(bool isDucking);
    void reset();

    std::vector<CollisionBox> getCollisionBoxes() const;

private:
    SDL_Renderer* renderer_;
    SDL_Texture*  sprite_;

    struct FrameInfo { std::vector<int> frames; float msPerFrame; };
    static const FrameInfo ANIM_FRAMES[];

    int   currentFrame_    = 0;
    float frameTimer_      = 0.0f;
    float animTimer_       = 0.0f;
    float blinkDelay_      = 0.0f;
    Uint32 animStartTime_  = 0;

    float jumpVelocity_    = 0.0f;
    bool  reachedMinHeight_= false;
    int   groundYPos_      = GROUND_Y;
    int   minJumpHeight_   = GROUND_Y - TREX_MIN_JUMP_HEIGHT;

    void setBlinkDelay();
    void blink(Uint32 now);
    void drawFrame(int xOffset, int yOffset) const;
};
