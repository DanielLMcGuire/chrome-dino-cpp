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

    Obstacle(SDL_Renderer* renderer,
             SDL_Texture* sprite,
             SDL_Texture* spriteInv,
             const ObstacleTypeDef* type,
             float currentSpeed);

    void update(float deltaTime, float speed, bool night);
    void draw(bool night) const;

    bool isVisible() const { return xPos + width > 0.0f; }

    std::vector<CollisionBox> collisionBoxes;

private:
    SDL_Renderer* renderer_;
    SDL_Texture*  sprite_;
    SDL_Texture*  spriteInv_;

    float speedOffset_  = 0.0f;
    int   currentFrame_ = 0;
    float frameTimer_   = 0.0f;

    void  init(float speed);
    float getGap(float speed) const;
    void  cloneCollisionBoxes();
};
