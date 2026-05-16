#pragma once
#include "defs.h"
#include "entities/trex.h"
#include "entities/horizon.h"
#include "entities/distance_meter.h"
#include "entities/game_over_panel.h"

#include <SDL_mixer.h>
#include <SDL2/SDL.h>
#include <memory>
#include <array>

enum class GameState { WAITING, PLAYING, PAUSED, GAME_OVER };

class Game {
public:
    /// @brief Constructs the game and initializes all of its components
    Game(SDL_Renderer* renderer, SDL_Texture* sprite, SDL_Texture* spriteInv);
    ~Game();

    /// @brief Handles all game events (keyboard, mouse, etc.)
    void handleEvent(const SDL_Event& e);
    /// @brief Updates the game state
    void update();

    /// @brief `running_` getter
    bool isRunning() const { return running_; }

    /// @brief `trex_` getter
    Trex*    getTrex()         { return trex_.get(); }
    /// @brief `horizon_` getter
    Horizon* getHorizon()      { return horizon_.get(); }
    /// @brief `currentSpeed_` getter
    float    getCurrentSpeed() { return currentSpeed_; }

private:
    /// @brief SDL Renderer
    SDL_Renderer* renderer_;
    /// @brief Spritesheet
    SDL_Texture*  sprite_;
    /// @brief Inverted Spritesheet
    SDL_Texture*  spriteInv_;

    /// @brief T-Rex (player)
    std::unique_ptr<Trex>         trex_;
    /// @brief Horizon (background)
    std::unique_ptr<Horizon>      horizon_;
    /// @brief Distance meter
    std::unique_ptr<DistanceMeter>distanceMeter_;
    /// @brief Game over panel
    std::unique_ptr<GameOverPanel>gameOverPanel_;

    /// @brief Hit (colision) sound
    Mix_Chunk* sndHit_   = nullptr;
    /// @brief Button press sound
    Mix_Chunk* sndPress_ = nullptr;
    /// @brief +100 score sound
    Mix_Chunk* sndScore_ = nullptr;

    /// @brief Game state
    GameState state_       = GameState::WAITING;
    /// @brief Game is running
    bool      running_     = true;
    /// @brief Jump keybind is pressed
    bool      keyJump_     = false;
    /// @brief Duck keybind is pressed
    bool      keyDuck_     = false;
    /// @brief Game is inverted (night mode)
    bool      inverted_    = false;

    /// @brief Player speed
    float currentSpeed_    = INITIAL_SPEED;
    /// @brief Distance ran
    float distanceRan_     = 0.0f;
    /// @brief Time running
    float runningTime_     = 0.0f;
    /// @brief Night mode timer
    float invertTimer_     = 0.0f;
    /// @brief Night mode should trigger
    bool  invertTrigger_   = false;
    /// @brief High score
    int   highestScore_    = 0;

    Uint32 lastTime_       = 0;
    Uint32 crashTime_      = 0;

    /// @brief Game controller
    SDL_GameController* gamepad_     = nullptr;
    /// @brief Previous gamepad state
    std::array<bool, 16> padPrev_    = {};

    /// @brief Start the game
    void startGame();
    /// @brief End the game
    void gameOver();
    /// @brief Restart the game
    void restart();
    /// @brief Clear the canvas
    void clearCanvas() const;
    /// @brief Check for collisions
    bool checkCollision() const;
    /// @brief Play a sound effect
    void playSound(Mix_Chunk* chunk);
    /// @brief Load sound effects
    void loadSounds();
    /// @brief Handle night mode (inverted colors)
    void handleNightMode(float deltaTime);
    /// @brief Poll gamepad input
    void pollGamepad();
    /// @brief Load high score
    void loadHighScore();
    /// @brief Save high score
    void saveHighScore();
};
