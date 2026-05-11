#pragma once
#include "defs.h"
#include "trex.h"
#include "horizon.h"
#include "distance_meter.h"
#include "game_over_panel.h"

#include <SDL_mixer.h>
#include <memory>

enum class GameState { WAITING, PLAYING, GAME_OVER };

class Game {
public:
    Game(SDL_Renderer* renderer, SDL_Texture* sprite, SDL_Texture* spriteInv);
    ~Game();

    void handleEvent(const SDL_Event& e);
    void update();

    bool isRunning() const { return running_; }

    Trex*    getTrex()         { return trex_.get(); }
    Horizon* getHorizon()      { return horizon_.get(); }
    float    getCurrentSpeed() { return currentSpeed_; }

private:
    SDL_Renderer* renderer_;
    SDL_Texture*  sprite_;
    SDL_Texture*  spriteInv_;

    std::unique_ptr<Trex>         trex_;
    std::unique_ptr<Horizon>      horizon_;
    std::unique_ptr<DistanceMeter>distanceMeter_;
    std::unique_ptr<GameOverPanel>gameOverPanel_;

    Mix_Chunk* sndHit_   = nullptr;
    Mix_Chunk* sndPress_ = nullptr;
    Mix_Chunk* sndScore_ = nullptr;

    GameState state_       = GameState::WAITING;
    bool      running_     = true;
    bool      keyJump_     = false;
    bool      keyDuck_     = false;
    bool      inverted_    = false;

    float currentSpeed_    = INITIAL_SPEED;
    float distanceRan_     = 0.0f;
    float runningTime_     = 0.0f;
    float invertTimer_     = 0.0f;
    bool  invertTrigger_   = false;
    int   highestScore_    = 0;

    Uint32 lastTime_       = 0;
    Uint32 crashTime_      = 0;

    void startGame();
    void gameOver();
    void restart();
    void clearCanvas() const;
    bool checkCollision() const;
    void playSound(Mix_Chunk* chunk);
    void loadSounds();
    void handleNightMode(float deltaTime);
};
