#include "obstacle.h"
#include "rand.h"
#include "drawSprite.h"
#include <cmath>

static constexpr ObstacleTypeDef s_cactusSmall = {
    "cactusSmall",
    17, 35,
    {105}, 1,       // yPos, yPosCount
    4.0f,           // multipleSpeed
    120,            // minGap
    0.0f,           // minSpeed
    0.0f,           // speedOffset
    {               // collisionBoxes
        { 0,  7,  5, 27},
        { 4,  0,  6, 34},
        {10,  4,  7, 14},
    }, 3,           // collisionBoxCount
    0, 0.0f,        // numFrames, frameRate
    SP_CACTUS_SMALL,
};

static constexpr ObstacleTypeDef s_cactusLarge = {
    "cactusLarge",
    25, 50,
    {90}, 1,
    7.0f,
    120,
    0.0f,
    0.0f,
    {
        { 0, 12,  7, 38},
        { 8,  0,  7, 49},
        {13, 10, 10, 38},
    }, 3,
    0, 0.0f,
    SP_CACTUS_LARGE,
};

static constexpr ObstacleTypeDef s_pterodactyl = {
    "pterodactyl",
    46, 40,
    {100, 75, 50}, 3,  // variable height
    999.0f,
    150,
    8.5f,
    0.8f,           // speedOffset
    {
        {15, 15, 16,  5},
        {18, 21, 24,  6},
        { 2, 14,  4,  3},
        { 6, 10,  4,  7},
        {10,  8,  6,  9},
    }, 5,
    2,              // numFrames
    1000.0f / 6.0f, // frameRate
    SP_PTERODACTYL,
};

const ObstacleTypeDef& getCactusSmallDef()   { return s_cactusSmall; }
const ObstacleTypeDef& getCactusLargeDef()   { return s_cactusLarge; }
const ObstacleTypeDef& getPterodactylDef()   { return s_pterodactyl; }

Obstacle::Obstacle(SDL_Renderer* renderer,
                   SDL_Texture* sprite,
                   SDL_Texture* spriteInv,
                   const ObstacleTypeDef* type,
                   float currentSpeed)
    : renderer_(renderer), sprite_(sprite), spriteInv_(spriteInv), typeConfig(type)
{
    cloneCollisionBoxes();
    xPos = (float)GAME_WIDTH;
    init(currentSpeed);
}

void Obstacle::init(float speed) {
    size = randInt(1, MAX_OBSTACLE_LEN);

    if (size > 1 && typeConfig->multipleSpeed > speed) {
        size = 1;
    }
    width = typeConfig->width * size;

    yPos = (float)typeConfig->yPos[randInt(0, typeConfig->yPosCount - 1)];

    if (typeConfig->speedOffset > 0.0f) {
        speedOffset_ = (randFloat() > 0.5f)
                          ?  typeConfig->speedOffset
                          : -typeConfig->speedOffset;
    }

    if (size > 1 && collisionBoxCount >= 3) {
        collisionBoxes[1].w = width
                             - collisionBoxes[0].w
                             - collisionBoxes[2].w;
        collisionBoxes[2].x = width - collisionBoxes[2].w;
    }

    gap = getGap(speed);
}

void Obstacle::cloneCollisionBoxes() {
    collisionBoxCount = typeConfig->collisionBoxCount;
    for (int i = 0; i < collisionBoxCount; ++i)
        collisionBoxes[i] = typeConfig->collisionBoxes[i];
}

float Obstacle::getGap(float speed) const {
    float minGap = std::round((float)width * speed
                              + (float)typeConfig->minGap * GAP_COEFFICIENT);
    float maxGap = std::round(minGap * MAX_GAP_COEFFICIENT);
    return (float)randInt((int)minGap, (int)maxGap);
}

void Obstacle::draw(bool night) const {
    SDL_Texture* tex = night ? spriteInv_ : sprite_;
    const SpritePos orig = typeConfig->spritePos;
    int sw = typeConfig->width;
    int sh = typeConfig->height;

    float srcXf = (float)(sw * size) * 0.5f * (float)(size - 1) + (float)orig.x;
    int srcX = (int)srcXf;
    int srcY = orig.y;

    if (currentFrame_ > 0) {
        srcX += sw * currentFrame_;
    }

    int dstW = typeConfig->width * size;
    int dstH = typeConfig->height;

    drawSprite(renderer_, tex,
               srcX, srcY, sw * size, sh,
               (int)xPos, (int)yPos, dstW, dstH);
}

void Obstacle::update(float deltaTime, float speed, bool night) {
    if (remove) return;

    float effectiveSpeed = speed + speedOffset_;
    xPos -= std::floor((effectiveSpeed * FPS / 1000.0f) * deltaTime);

    if (typeConfig->numFrames > 0) {
        frameTimer_ += deltaTime;
        if (frameTimer_ >= typeConfig->frameRate) {
            currentFrame_ = (currentFrame_ + 1) % typeConfig->numFrames;
            frameTimer_   = 0.0f;
        }
    }

    draw(night);

    if (!isVisible()) {
        remove = true;
    }
}
