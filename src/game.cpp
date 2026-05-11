#include "game.h"
#include <cmath>
#include <algorithm>

Game::Game(SDL_Renderer* renderer, SDL_Texture* sprite, SDL_Texture* spriteInv)
    : renderer_(renderer), sprite_(sprite), spriteInv_(spriteInv)
{
    trex_          = std::make_unique<Trex>(renderer_, sprite_, spriteInv_);
    horizon_       = std::make_unique<Horizon>(renderer_, sprite_, spriteInv_);
    distanceMeter_ = std::make_unique<DistanceMeter>(renderer_, sprite_, spriteInv_);
    gameOverPanel_ = std::make_unique<GameOverPanel>(renderer_, sprite_, spriteInv_);

    loadSounds();
    lastTime_ = SDL_GetTicks();
}

Game::~Game() {
    if (sndHit_)   Mix_FreeChunk(sndHit_);
    if (sndPress_) Mix_FreeChunk(sndPress_);
    if (sndScore_) Mix_FreeChunk(sndScore_);
}

void Game::loadSounds() {
    sndHit_   = Mix_LoadWAV("resources/sounds/hit.mp3");
    sndPress_ = Mix_LoadWAV("resources/sounds/button-press.mp3");
    sndScore_ = Mix_LoadWAV("resources/sounds/score-reached.mp3");
}

void Game::playSound(Mix_Chunk* chunk) {
    if (chunk) Mix_PlayChannel(-1, chunk, 0);
}

void Game::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_QUIT) {
        running_ = false;
        return;
    }
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_ESCAPE:
                running_ = false;
                break;

            case SDLK_UP:
            case SDLK_SPACE:
                if (state_ == GameState::WAITING || state_ == GameState::PLAYING) {
                    if (!keyJump_) {
                        keyJump_ = true;
                        if (state_ == GameState::WAITING) {
                            startGame();
                        }
                        if (!trex_->jumping && !trex_->ducking) {
                            playSound(sndPress_);
                            trex_->startJump(currentSpeed_);
                        }
                    }
                } else if (state_ == GameState::GAME_OVER) {
                    Uint32 elapsed = SDL_GetTicks() - crashTime_;
                    if (elapsed >= GAMEOVER_CLEAR_TIME) {
                        restart();
                    }
                }
                break;

            case SDLK_DOWN:
                if (!keyDuck_) {
                    keyDuck_ = true;
                    if (state_ == GameState::PLAYING) {
                        if (trex_->jumping) {
                            trex_->setSpeedDrop();
                        } else if (!trex_->jumping && !trex_->ducking) {
                            trex_->setDuck(true);
                        }
                    }
                }
                break;

            case SDLK_RETURN:
                if (state_ == GameState::GAME_OVER) {
                    restart();
                }
                break;

            default: break;
        }
    }
    if (e.type == SDL_KEYUP) {
        switch (e.key.keysym.sym) {
            case SDLK_UP:
            case SDLK_SPACE:
                keyJump_ = false;
                trex_->endJump();
                break;
            case SDLK_DOWN:
                keyDuck_ = false;
                trex_->speedDrop = false;
                trex_->setDuck(false);
                break;
            default: break;
        }
    }
}

void Game::startGame() {
    state_        = GameState::PLAYING;
    runningTime_  = 0.0f;
    distanceRan_  = 0.0f;
    currentSpeed_ = INITIAL_SPEED;
}

void Game::gameOver() {
    playSound(sndHit_);

    state_     = GameState::GAME_OVER;
    crashTime_ = SDL_GetTicks();

    trex_->update(0.0f, TrexStatus::CRASHED);
    distanceMeter_->achievement = false;

    if (distanceRan_ > (float)highestScore_) {
        highestScore_ = (int)distanceRan_;
        distanceMeter_->setHighScore(highestScore_);
    }
}

void Game::restart() {
    state_        = GameState::PLAYING;
    runningTime_  = 0.0f;
    distanceRan_  = 0.0f;
    currentSpeed_ = INITIAL_SPEED;
    inverted_     = false;
    invertTimer_  = 0.0f;

    trex_->reset();
    horizon_->reset();
    distanceMeter_->reset();
    gameOverPanel_->reset();
    playSound(sndPress_);
}

void Game::clearCanvas() const {
    if (inverted_) {
        SDL_SetRenderDrawColor(renderer_, 83, 83, 83, 255);
    } else {
        SDL_SetRenderDrawColor(renderer_, 247, 247, 247, 255);
    }
    SDL_RenderClear(renderer_);
}

void Game::handleNightMode(float deltaTime) {
    if (invertTimer_ > INVERT_FADE_DURATION) {
        invertTimer_  = 0.0f;
        invertTrigger_= false;
        inverted_     = false;
    } else if (invertTimer_ > 0.0f) {
        invertTimer_ += deltaTime;
    } else {
        int actualDist = distanceMeter_->getActualDistance(distanceRan_);
        if (actualDist > 0) {
            invertTrigger_ = (actualDist % (int)INVERT_DISTANCE == 0);
            if (invertTrigger_ && invertTimer_ == 0.0f) {
                invertTimer_ += deltaTime;
                inverted_ = !inverted_;
            }
        }
    }
}

bool Game::checkCollision() const {
    if (horizon_->obstacles.empty()) return false;
    const auto& obs = *horizon_->obstacles[0];

    CollisionBox tRexBox = {
        (int)trex_->xPos + 1,
        (int)trex_->yPos + 1,
        TREX_WIDTH - 2,
        TREX_HEIGHT - 2
    };
    CollisionBox obsBox = {
        (int)obs.xPos + 1,
        (int)obs.yPos + 1,
        obs.typeConfig->width * obs.size - 2,
        obs.typeConfig->height - 2
    };

    if (!boxesOverlap(tRexBox, obsBox)) return false;

    const auto& tRexBoxes = trex_->getCollisionBoxes();
    for (const auto& tb : tRexBoxes) {
        CollisionBox adjT = adjustedBox(tb, tRexBox);
        for (const auto& ob : obs.collisionBoxes) {
            CollisionBox adjO = adjustedBox(ob, obsBox);
            if (boxesOverlap(adjT, adjO)) return true;
        }
    }
    return false;
}

void Game::update() {
    Uint32 now       = SDL_GetTicks();
    float  deltaTime = (float)(now - lastTime_);
    lastTime_        = now;

    clearCanvas();

    if (state_ == GameState::WAITING) {
        horizon_->update(0.0f, currentSpeed_, false, false, false);
        if (trex_->blinkCount < MAX_BLINK_COUNT) {
            trex_->update(deltaTime, TrexStatus(-1), false);
        }
        distanceMeter_->update(deltaTime, 0, false);

    } else if (state_ == GameState::PLAYING) {
        runningTime_ += deltaTime;
        bool hasObstacles = runningTime_ > (float)CLEAR_TIME;

        if (trex_->jumping) {
            trex_->updateJump(deltaTime);
        }

        bool showNight = inverted_;
        horizon_->update(deltaTime, currentSpeed_, hasObstacles, showNight, showNight);
        trex_->update(deltaTime, TrexStatus(-1), showNight);

        if (hasObstacles && checkCollision()) {
            gameOver();
        } else {
            distanceRan_ += currentSpeed_ * deltaTime / MS_PER_FRAME;
            if (currentSpeed_ < MAX_SPEED) {
                currentSpeed_ += ACCELERATION;
            }
        }

        bool playScore = distanceMeter_->update(deltaTime,
                                                (int)std::ceil(distanceRan_), showNight);
        if (playScore) {
            playSound(sndScore_);
        }

        handleNightMode(deltaTime);

    } else if (state_ == GameState::GAME_OVER) {
        horizon_->update(0.0f, 0.0f, false, inverted_, inverted_);
        horizon_->draw(inverted_);
        trex_->update(0.0f, TrexStatus(-1), inverted_);
        distanceMeter_->update(0.0f, (int)std::ceil(distanceRan_), inverted_);
        gameOverPanel_->update(deltaTime, inverted_);
    }
}
