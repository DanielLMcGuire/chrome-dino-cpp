#pragma once
#include "defs.h"
#include "cloud.h"
#include "horizon_line.h"
#include "night_mode.h"
#include "obstacle.h"
#include <vector>
#include <memory>

class Horizon {
public:
    std::vector<std::unique_ptr<Obstacle>> obstacles;

    Horizon(SDL_Renderer* r, SDL_Texture* t, SDL_Texture* ti)
        : renderer_(r), sprite_(t), spriteInv_(ti),
          horizonLine_(r, t, ti),
          nightMode_(r, t, ti)
    {
        addCloud();
    }

    void update(float deltaTime, float speed, bool updateObstacles, bool nightMode, bool night) {
        horizonLine_.update(deltaTime, speed, night);
        updateClouds(deltaTime, speed, night);
        nightMode_.update(nightMode, night);
        if (updateObstacles) {
            updateObstacleList(deltaTime, speed, night);
        }
    }

    void reset() {
        obstacles.clear();
        clouds_.clear();
        horizonLine_.reset();
        nightMode_.reset();
        addCloud();
    }

    void removeFirstObstacle() {
        if (!obstacles.empty()) obstacles.erase(obstacles.begin());
    }

    void draw(bool night) const {
        horizonLine_.draw(night);
        for (const auto& obs : obstacles)
            obs->draw(night);
    }

private:
    SDL_Renderer* renderer_;
    SDL_Texture*  sprite_;
    SDL_Texture*  spriteInv_;

    HorizonLine horizonLine_;
    NightMode   nightMode_;

    std::vector<std::unique_ptr<Cloud>> clouds_;
    float cloudSpeed_ = BG_CLOUD_SPEED;

    std::vector<const ObstacleTypeDef*> obstacleTypes_;
    std::string lastObstacleType_;

    void addCloud() {
        clouds_.push_back(std::make_unique<Cloud>(renderer_, sprite_, spriteInv_));
    }

    void updateClouds(float deltaTime, float speed, bool night) {
        float cloudSpeed = cloudSpeed_ / 1000.0f * deltaTime * speed;
        for (auto& c : clouds_) {
            c->update(cloudSpeed, night);
        }
        clouds_.erase(std::remove_if(clouds_.begin(), clouds_.end(),
                       [](const auto& c) { return c->remove; }),
                      clouds_.end());

        if ((int)clouds_.size() < MAX_CLOUDS) {
            if (clouds_.empty()
                || (GAME_WIDTH - clouds_.back()->xPos) > clouds_.back()->gap) {
                if (randFloat() < CLOUD_FREQUENCY) {
                    addCloud();
                }
            }
        }
    }

    void updateObstacleList(float deltaTime, float speed, bool night) {
        for (auto& obs : obstacles) {
            obs->update(deltaTime, speed, night);
        }

        obstacles.erase(
            std::remove_if(obstacles.begin(), obstacles.end(),
                           [](const auto& o) { return o->remove; }),
            obstacles.end());

        if (obstacles.empty()) {
            addNewObstacle(speed);
        } else {
            const auto& last = obstacles.back();
            if (last->followingObstacleCreated) return;

            bool readyToSpawn =
                last->xPos + last->width + last->gap < GAME_WIDTH;
            if (readyToSpawn) {
                last->followingObstacleCreated = true;
                addNewObstacle(speed);
            }
        }
    }

    void addNewObstacle(float speed) {
        std::vector<const ObstacleTypeDef*> candidates;
        if (speed >= getCactusSmallDef().minSpeed)
            candidates.push_back(&getCactusSmallDef());
        if (speed >= getCactusLargeDef().minSpeed)
            candidates.push_back(&getCactusLargeDef());
        if (speed >= getPterodactylDef().minSpeed)
            candidates.push_back(&getPterodactylDef());

        if (candidates.empty()) candidates.push_back(&getCactusSmallDef());

        const ObstacleTypeDef* chosen = nullptr;
        if (candidates.size() == 1 || lastObstacleType_.empty()) {
            chosen = candidates[randInt(0, (int)candidates.size() - 1)];
        } else {
            std::vector<const ObstacleTypeDef*> filtered;
            for (auto c : candidates)
                if (std::string(c->type) != lastObstacleType_)
                    filtered.push_back(c);
            if (filtered.empty()) filtered = candidates;
            chosen = filtered[randInt(0, (int)filtered.size() - 1)];
        }

        lastObstacleType_ = chosen->type;
        obstacles.push_back(
            std::make_unique<Obstacle>(renderer_, sprite_, spriteInv_, chosen, speed));
    }
};
