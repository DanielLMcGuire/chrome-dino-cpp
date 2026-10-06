#pragma once
#include "defs.h"
#include "cloud.h"
#include "horizon_line.h"
#include "night_mode.h"
#include "obstacle.h"
#include "colbox.h"
#include "util.h"
#include "static_vector.h"

class Horizon {
public:
    static constexpr int MAX_OBSTACLES = 8;

    StaticVector<Obstacle, MAX_OBSTACLES> obstacles;

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
        historyCount_ = 0;
        horizonLine_.reset();
        nightMode_.reset();
        addCloud();
    }

    void removeFirstObstacle() {
        obstacles.pop_front();
    }

    void draw(bool night) const {
        horizonLine_.draw(night);
        drawObstacles(night);
    }

    void drawObstacles(bool night) const {
        for (const auto& obs : obstacles)
            obs.draw(night);
    }

private:
    SDL_Renderer* renderer_;
    SDL_Texture*  sprite_;
    SDL_Texture*  spriteInv_;

    HorizonLine horizonLine_;
    NightMode   nightMode_;

    StaticVector<Cloud, MAX_CLOUDS> clouds_;
    float cloudSpeed_ = BG_CLOUD_SPEED;

    static constexpr int MAX_OBSTACLE_DUPLICATION = 2;

    const ObstacleTypeDef* history_[MAX_OBSTACLE_DUPLICATION] = {};
    int historyCount_ = 0;

    void addCloud() {
        clouds_.push_back(Cloud(renderer_, sprite_, spriteInv_));
    }

    void updateClouds(float deltaTime, float speed, bool night) {
        float cloudSpeed = cloudSpeed_ / 1000.0f * deltaTime * speed;
        for (auto& c : clouds_) {
            c.update(cloudSpeed, night);
        }
        clouds_.remove_if([](const Cloud& c) { return c.remove; });

        if ((int)clouds_.size() < MAX_CLOUDS) {
            if (clouds_.empty()
                || (GAME_WIDTH - clouds_.back().xPos) > clouds_.back().gap) {
                if (randFloat() < CLOUD_FREQUENCY) {
                    addCloud();
                }
            }
        }
    }

    void updateObstacleList(float deltaTime, float speed, bool night) {
        for (auto& obs : obstacles) {
            obs.update(deltaTime, speed, night);
        }

        obstacles.remove_if([](const Obstacle& o) { return o.remove; });

        if (obstacles.empty()) {
            addNewObstacle(speed);
        } else {
            auto& last = obstacles.back();
            if (last.followingObstacleCreated) return;

            bool readyToSpawn =
                last.xPos + (float)last.width + last.gap < GAME_WIDTH;
            if (readyToSpawn) {
                last.followingObstacleCreated = true;
                addNewObstacle(speed);
            }
        }
    }

    [[nodiscard]] bool duplicateObstacleCheck(const ObstacleTypeDef* type) const {
        int count = 0;
        for (int i = 0; i < historyCount_; ++i)
            count = (history_[i] == type) ? count + 1 : 0;
        return count >= MAX_OBSTACLE_DUPLICATION;
    }

    void pushHistory(const ObstacleTypeDef* type) {
        int n = historyCount_ < MAX_OBSTACLE_DUPLICATION ? historyCount_ + 1
                                                         : MAX_OBSTACLE_DUPLICATION;
        for (int i = n - 1; i > 0; --i) history_[i] = history_[i - 1];
        history_[0]   = type;
        historyCount_ = n;
    }

    void addNewObstacle(float speed) {
        const ObstacleTypeDef* candidates[3];
        int numCandidates = 0;
        if (speed >= getCactusSmallDef().minSpeed)
            candidates[numCandidates++] = &getCactusSmallDef();
        if (speed >= getCactusLargeDef().minSpeed)
            candidates[numCandidates++] = &getCactusLargeDef();
        if (speed >= getPterodactylDef().minSpeed)
            candidates[numCandidates++] = &getPterodactylDef();

        if (numCandidates == 0) candidates[numCandidates++] = &getCactusSmallDef();

        const ObstacleTypeDef* chosen;
        do {
            chosen = candidates[randInt(0, numCandidates - 1)];
        } while (numCandidates > 1 && duplicateObstacleCheck(chosen));

        pushHistory(chosen);

        obstacles.push_back(Obstacle(renderer_, sprite_, spriteInv_, chosen, speed));
    }
};
