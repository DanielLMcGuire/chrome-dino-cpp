#pragma once
#include "defs.h"
#include "entities/trex.h"
#include "entities/horizon.h"

// This file is ported from
// https://mathewsachin.github.io/blog/2026/03/14/chrome-dino-autoplay.html

class AutoPlayer {
public:
    bool enabled = false;

    void tick(const Trex& trex, const Horizon& horizon, float speed) {
        if (!enabled) return;

        if (trex.status == TrexStatus::WAITING) {
            pushKey(SDLK_SPACE, true);
            pushKey(SDLK_SPACE, false);
            return;
        }

        if (trex.status == TrexStatus::CRASHED) {
            if (++restartTick_ > 90) {
                pushKey(SDLK_SPACE, true);
                pushKey(SDLK_SPACE, false);
                restartTick_ = 0;
                releaseDuck();
            }
            return;
        }
        restartTick_ = 0;

        if (horizon.obstacles.empty()) {
            releaseDuck();
            return;
        }

        const Obstacle& obs = *horizon.obstacles[0];

        float x          = obs.xPos;
        float y          = obs.yPos;
        float w          = (float)obs.width;
        float obsHeight  = (float)obs.typeConfig->height;
        float yFromBottom = GAME_HEIGHT - y - obsHeight;

        bool isNearby = x < 25.0f * speed - w / 2.0f;

        if (!isNearby) {
            releaseDuck();
            return;
        }

        if (yFromBottom > (float)TREX_HEIGHT) {
            releaseDuck();
        } else if (y > (float)GAME_HEIGHT / 2.0f) {
            releaseDuck();
            if (!trex.jumping) {
                pushKey(SDLK_SPACE, true);
                pushKey(SDLK_SPACE, false);
            }
        } else {
            if (!duckKeyHeld_) {
                pushKey(SDLK_DOWN, true);
                duckKeyHeld_ = true;
            }
        }
    }

private:
    bool duckKeyHeld_ = false;
    int  restartTick_ = 0;

    void releaseDuck() {
        if (duckKeyHeld_) {
            pushKey(SDLK_DOWN, false);
            duckKeyHeld_ = false;
        }
    }

    static void pushKey(SDL_Keycode key, bool down) {
        SDL_Event e{};
        e.type           = down ? SDL_KEYDOWN : SDL_KEYUP;
        e.key.keysym.sym = key;
        e.key.state      = down ? SDL_PRESSED : SDL_RELEASED;
        e.key.repeat     = 0;
        SDL_PushEvent(&e);
    }
};