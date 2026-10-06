#pragma once
#include "defs.h"

class Obstacle {
public:
    float xPos = 0.0f;
    float yPos = 0.0f;
    int   size = 1;
    int   width = 0;
    bool  remove = false;
    float gap    = 0.0f;
    bool  followingObstacleCreated = false;

    const ObstacleTypeDef* typeConfig = nullptr;

    Obstacle() = default;

    Obstacle(SDL_Renderer* renderer,
             SDL_Texture* sprite,
             SDL_Texture* spriteInv,
             const ObstacleTypeDef* type,
             float currentSpeed);

    void update(float deltaTime, float speed, bool night);
    void draw(bool night) const;

    [[nodiscard]] bool isVisible() const { return xPos + (float)width > 0.0f; }

    CollisionBox collisionBoxes[MAX_TYPE_BOXES] = {};
    int          collisionBoxCount = 0;

    [[nodiscard]] BoxSpan boxes() const { return { collisionBoxes, collisionBoxCount }; }

private:
    SDL_Renderer* renderer_  = nullptr;
    SDL_Texture*  sprite_    = nullptr;
    SDL_Texture*  spriteInv_ = nullptr;

    float speedOffset_  = 0.0f;
    int   currentFrame_ = 0;
    float frameTimer_   = 0.0f;

    void  init(float speed);
    [[nodiscard]] float getGap(float speed) const;
    void  cloneCollisionBoxes();
};
