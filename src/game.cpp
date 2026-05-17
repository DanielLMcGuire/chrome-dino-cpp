#include "game.h"
#include "util.h"
#ifdef UWP
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#endif
#include <cmath>
#include <algorithm>
#include <fstream>

Game::Game(SDL_Renderer* renderer, SDL_Texture* sprite, SDL_Texture* spriteInv)
    : renderer_(renderer), sprite_(sprite), spriteInv_(spriteInv)
{
    trex_          = std::make_unique<Trex>(renderer_, sprite_, spriteInv_);
    horizon_       = std::make_unique<Horizon>(renderer_, sprite_, spriteInv_);
    distanceMeter_ = std::make_unique<DistanceMeter>(renderer_, sprite_, spriteInv_);
    gameOverPanel_ = std::make_unique<GameOverPanel>(renderer_, sprite_, spriteInv_);

    loadSounds();
    loadHighScore();
    lastTime_ = SDL_GetTicks();
}

Game::~Game() {
    if (sndHit_)   Mix_FreeChunk(sndHit_);
    if (sndPress_) Mix_FreeChunk(sndPress_);
    if (sndScore_) Mix_FreeChunk(sndScore_);
    if (gamepad_)  SDL_GameControllerClose(gamepad_);
}

void Game::loadSounds() {
#ifdef UWP
    {
        auto bytes = LoadFileBytes(hitWav);
        SDL_RWops* rw = SDL_RWFromMem(bytes.data(), (int)bytes.size());
        sndHit_ = Mix_LoadWAV_RW(rw, 1);
    }
    {
        auto bytes = LoadFileBytes(pressWav);
        SDL_RWops* rw = SDL_RWFromMem(bytes.data(), (int)bytes.size());
        sndPress_ = Mix_LoadWAV_RW(rw, 1);
    }
    {
        auto bytes = LoadFileBytes(scoreWav);
        SDL_RWops* rw = SDL_RWFromMem(bytes.data(), (int)bytes.size());
        sndScore_ = Mix_LoadWAV_RW(rw, 1);
    }
#else
    sndHit_   = Mix_LoadWAV(hitWav);
    sndPress_ = Mix_LoadWAV(pressWav);
    sndScore_ = Mix_LoadWAV(scoreWav);
#endif
}

void Game::loadHighScore() {
#ifdef UWP
    auto localFolder = winrt::Windows::Storage::ApplicationData::Current().LocalFolder();
    std::wstring path(localFolder.Path().c_str());
    path += L"\\highscore.dat";
    std::ifstream f(path, std::ios::binary);
#else
    std::ifstream f("highscore.dat", std::ios::binary);
#endif
    if (!f) return;
    f.read(reinterpret_cast<char*>(&highestScore_), sizeof(highestScore_));
    if (f && highestScore_ > 0)
        distanceMeter_->setHighScore(highestScore_);
}

void Game::saveHighScore() {
#ifdef UWP
    auto localFolder = winrt::Windows::Storage::ApplicationData::Current().LocalFolder();
    std::wstring path(localFolder.Path().c_str());
    path += L"\\highscore.dat";
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
#else
    std::ofstream f("highscore.dat", std::ios::binary | std::ios::trunc);
#endif
    f.write(reinterpret_cast<const char*>(&highestScore_), sizeof(highestScore_));
}

void Game::playSound(Mix_Chunk* chunk) {
    if (chunk) Mix_PlayChannel(-1, chunk, 0);
}

void Game::pollGamepad() {
    if (!gamepad_) return;

    constexpr SDL_GameControllerButton BTN_JUMP    = SDL_CONTROLLER_BUTTON_A;
    constexpr SDL_GameControllerButton BTN_DUCK    = SDL_CONTROLLER_BUTTON_X;
    constexpr SDL_GameControllerButton BTN_RESTART = SDL_CONTROLLER_BUTTON_START;
    constexpr SDL_GameControllerButton BTN_CLEAR_HISCORE = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;

    auto pressed = [&](SDL_GameControllerButton idx) -> bool {
        return SDL_GameControllerGetButton(gamepad_,
            idx) != 0;
    };

    auto rising  = [&](SDL_GameControllerButton idx) { return  pressed(idx) && !padPrev_[idx]; };
    auto falling = [&](SDL_GameControllerButton idx) { return !pressed(idx) &&  padPrev_[idx]; };

    if (rising(BTN_JUMP)) {
        if (state_ == GameState::WAITING || state_ == GameState::PLAYING) {
            if (!keyJump_) {
                keyJump_ = true;
                if (state_ == GameState::WAITING) startGame();
                if (!trex_->jumping && !trex_->ducking) {
                    playSound(sndPress_);
                    trex_->startJump(currentSpeed_);
                }
            }
        } else if (state_ == GameState::GAME_OVER) {
            if (SDL_GetTicks() - crashTime_ >= GAMEOVER_CLEAR_TIME) restart();
        }
    }
    if (falling(BTN_JUMP)) {
        keyJump_ = false;
        trex_->endJump();
    }

    if (rising(BTN_DUCK)) {
        if (!keyDuck_ && state_ == GameState::PLAYING) {
            keyDuck_ = true;
            if (trex_->jumping)                          trex_->setSpeedDrop();
            else if (!trex_->jumping && !trex_->ducking) trex_->setDuck(true);
        }
    }

    if (falling(BTN_DUCK)) {
        keyDuck_         = false;
        trex_->speedDrop = false;
        trex_->setDuck(false);
    }
    
    if (rising(BTN_RESTART)) {
        if (state_ == GameState::WAITING) startGame();
        if (state_ == GameState::GAME_OVER) restart();
    }

    if (rising(BTN_CLEAR_HISCORE) && state_ == GameState::GAME_OVER) {
        Uint32 elapsed = SDL_GetTicks() - crashTime_;
        if (elapsed >= GAMEOVER_CLEAR_TIME) {
            if (distanceMeter_->isHighScoreFlashing()) {
                highestScore_ = 0;
                saveHighScore();
                distanceMeter_->resetHighScore();
            } else {
                distanceMeter_->startHighScoreFlashing();
            }
        }
    }

    for (int i = 0; i < (int)padPrev_.size(); ++i)
        padPrev_[i] = pressed(static_cast<SDL_GameControllerButton>(i));
}

void Game::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_QUIT) {
        running_ = false;
        return;
    }
    if (e.type == SDL_CONTROLLERDEVICEADDED && !gamepad_) {
        gamepad_ = SDL_GameControllerOpen(e.cdevice.which);
    }
    if (e.type == SDL_CONTROLLERDEVICEREMOVED && gamepad_) {
        if (SDL_GameControllerFromInstanceID(e.cdevice.which) == gamepad_) {
            SDL_GameControllerClose(gamepad_);
            gamepad_ = nullptr;
        }
    }
    if (e.type == SDL_WINDOWEVENT) {
        if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST ||
            e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
            if (state_ == GameState::PLAYING) {
                state_ = GameState::PAUSED;
            }
        } else if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
            if (state_ == GameState::PAUSED) {
                state_    = GameState::PLAYING;
                lastTime_ = SDL_GetTicks();
            }
        }
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        if (state_ == GameState::WAITING || state_ == GameState::PLAYING) {
            if (!keyJump_) {
                keyJump_ = true;
                if (state_ == GameState::WAITING) startGame();
                if (!trex_->jumping && !trex_->ducking) {
                    playSound(sndPress_);
                    trex_->startJump(currentSpeed_);
                }
            }
        }
    }
    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        keyJump_ = false;
        trex_->endJump();
        if (state_ == GameState::GAME_OVER) {
            Uint32 elapsed = SDL_GetTicks() - crashTime_;
            if (elapsed >= GAMEOVER_CLEAR_TIME) {
                int lx = e.button.x;
                int ly = e.button.y;
                SDL_Rect r = distanceMeter_->getHighScoreRect();
                bool onHiScore = highestScore_ > 0 &&
                                 lx >= r.x && lx < r.x + r.w &&
                                 ly >= r.y && ly < r.y + r.h;
                if (onHiScore) {
                    if (distanceMeter_->isHighScoreFlashing()) {
                        highestScore_ = 0;
                        saveHighScore();
                        distanceMeter_->resetHighScore();
                    } else {
                        distanceMeter_->startHighScoreFlashing();
                    }
                } else {
                    distanceMeter_->cancelHighScoreFlashing();
                    restart();
                }
            }
        }
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
                if (state_ == GameState::WAITING) {
                    startGame();
                } else if (state_ == GameState::GAME_OVER) {
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
    inverted_ = false;
    invertTimer_ = 0.0f;
    trex_->update(0.0f, TrexStatus::RUNNING, false);
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
        saveHighScore();
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
    distanceMeter_->cancelHighScoreFlashing();
    gameOverPanel_->reset();
    playSound(sndPress_);
}

void Game::clearCanvas() const {
    if (inverted_) {
        const auto [r, g, b, a] = HexToRGBA(INV_CANVAS);
        SDL_SetRenderDrawColor(renderer_, r, g, b, a);
    } else {
        const auto [r, g, b, a] = HexToRGBA(DAY_CANVAS);
        SDL_SetRenderDrawColor(renderer_, r, g, b, a);
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

    CollisionBox tRexBox;
    if (trex_->ducking) {
        tRexBox = {
            (int)trex_->xPos + 1,
            (int)trex_->yPos + TREX_HEIGHT - TREX_HEIGHT_DUCK + 1,
            TREX_WIDTH_DUCK - 2,
            TREX_HEIGHT_DUCK - 2
        };
    } else {
        tRexBox = {
            (int)trex_->xPos + 1,
            (int)trex_->yPos + 1,
            TREX_WIDTH - 2,
            TREX_HEIGHT - 2
        };
    }
    CollisionBox obsBox = {
        (int)obs.xPos + 1,
        (int)obs.yPos + 1,
        obs.typeConfig->width * obs.size - 2,
        obs.typeConfig->height - 2
    };

    if (!boxesOverlap(tRexBox, obsBox)) return false;

    auto tRexBoxes = trex_->getCollisionBoxes();
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
    pollGamepad();

    Uint32 now       = SDL_GetTicks();
    float  deltaTime = (float)(now - lastTime_);
    if (deltaTime > MS_PER_FRAME * 5.0f) deltaTime = MS_PER_FRAME;
    lastTime_        = now;

    clearCanvas();

    if (state_ == GameState::WAITING) {
        horizon_->update(0.0f, currentSpeed_, false, false, false);
        trex_->update(deltaTime, TrexStatus(-1), false);
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

    } else if (state_ == GameState::PAUSED) {
        horizon_->update(0.0f, 0.0f, false, inverted_, inverted_);
        horizon_->draw(inverted_);
        trex_->update(0.0f, TrexStatus(-1), inverted_);
        distanceMeter_->update(0.0f, (int)std::ceil(distanceRan_), inverted_);

    } else if (state_ == GameState::GAME_OVER) {
        horizon_->update(0.0f, 0.0f, false, inverted_, inverted_);
        horizon_->draw(inverted_);
        trex_->update(0.0f, TrexStatus(-1), inverted_);
        distanceMeter_->update(deltaTime, (int)std::ceil(distanceRan_), inverted_);
        gameOverPanel_->update(deltaTime, inverted_);
    }
}
