#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <algorithm>

constexpr int GAME_WIDTH  = 600;
constexpr int GAME_HEIGHT = 150;
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;

constexpr int   FPS            = 60;
constexpr float MS_PER_FRAME   = 1000.0f / FPS;

constexpr float INITIAL_SPEED       = 6.0f;
constexpr float MAX_SPEED           = 13.0f;
constexpr float ACCELERATION        = 0.001f;
constexpr float GAP_COEFFICIENT     = 0.6f;
constexpr float MAX_GAP_COEFFICIENT = 1.5f;
constexpr int   MAX_OBSTACLE_LEN    = 3;
constexpr int   CLEAR_TIME          = 3000;   // ms before obstacles appear
constexpr int   GAMEOVER_CLEAR_TIME = 1200;   // ms before restart allowed
constexpr float BOTTOM_PAD          = 10.0f;
constexpr float INVERT_DISTANCE     = 700.0f;
constexpr float INVERT_FADE_DURATION= 12000.0f;
constexpr int   MAX_BLINK_COUNT     = 3;
constexpr int   MAX_CLOUDS          = 6;
constexpr float CLOUD_FREQUENCY     = 0.5f;
constexpr float BG_CLOUD_SPEED      = 0.2f;

constexpr int   TREX_WIDTH              = 44;
constexpr int   TREX_HEIGHT             = 47;
constexpr int   TREX_WIDTH_DUCK         = 59;
constexpr int   TREX_HEIGHT_DUCK        = 25;
constexpr int   TREX_START_X            = 50;
constexpr float TREX_GRAVITY            = 0.6f;
constexpr float TREX_INITIAL_JUMP_VEL   = -10.0f;
constexpr float TREX_DROP_VEL           = -5.0f;
constexpr int   TREX_MIN_JUMP_HEIGHT    = 30;
constexpr int   TREX_MAX_JUMP_HEIGHT    = 30;
constexpr float TREX_SPEED_DROP_COEFF   = 3.0f;
constexpr float TREX_FLASH_ON           = 100.0f;
constexpr float TREX_FLASH_OFF          = 175.0f;

constexpr int GROUND_Y = GAME_HEIGHT - TREX_HEIGHT - (int)BOTTOM_PAD;

struct SpritePos { int x, y; };

constexpr SpritePos SP_TREX          = {848, 2};
constexpr SpritePos SP_CACTUS_SMALL  = {228, 2};
constexpr SpritePos SP_CACTUS_LARGE  = {332, 2};
constexpr SpritePos SP_PTERODACTYL   = {134, 2};
constexpr SpritePos SP_CLOUD         = { 86, 2};
constexpr SpritePos SP_MOON          = {484, 2};
constexpr SpritePos SP_STAR          = {645, 2};
constexpr SpritePos SP_RESTART       = {  2,68};
constexpr SpritePos SP_TEXT          = {655, 2};
constexpr SpritePos SP_HORIZON       = {  2,52};

struct CollisionBox { int x, y, w, h; };

inline CollisionBox adjustedBox(const CollisionBox& box, const CollisionBox& origin) {
    return {box.x + origin.x, box.y + origin.y, box.w, box.h};
}

inline bool boxesOverlap(const CollisionBox& a, const CollisionBox& b) {
    return a.x < b.x + b.w  &&
           a.x + a.w > b.x  &&
           a.y < b.y + b.h  &&
           a.y + a.h > b.y;
}

struct ObstacleTypeDef {
    const char* type;
    int   width;
    int   height;
    std::vector<int> yPos;
    float multipleSpeed;
    int   minGap;
    float minSpeed;
    float speedOffset;
    std::vector<CollisionBox> collisionBoxes;
    int   numFrames;
    float frameRate;
};

const ObstacleTypeDef& getCactusSmallDef();
const ObstacleTypeDef& getCactusLargeDef();
const ObstacleTypeDef& getPterodactylDef();

inline float randFloat() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

inline int randInt(int lo, int hi) {
    if (lo >= hi) return lo;
    return lo + std::rand() % (hi - lo + 1);
}

inline bool IS_HIDPI = false;

inline void drawSprite(SDL_Renderer* r, SDL_Texture* t,
                       int sx, int sy, int sw, int sh,
                       int dx, int dy,
                       int dw = -1, int dh = -1,
                       double angle = 0.0,
                       SDL_RendererFlip flip = SDL_FLIP_NONE)
{
    if (IS_HIDPI) {
        sx *= 2; sy *= 2; sw *= 2; sh *= 2;
    }
    SDL_Rect src = { sx, sy, sw, sh };
    SDL_Rect dst = {
        dx,
        dy,
        (dw < 0 ? sw / (IS_HIDPI ? 2 : 1) : dw),
        (dh < 0 ? sh / (IS_HIDPI ? 2 : 1) : dh)
    };
    if (angle != 0.0 || flip != SDL_FLIP_NONE)
        SDL_RenderCopyEx(r, t, &src, &dst, angle, nullptr, flip);
    else
        SDL_RenderCopy(r, t, &src, &dst);
}

inline SDL_Surface* createInvertedSurface(SDL_Surface* src) {
    SDL_Surface* conv = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA32, 0);
    if (!conv) return nullptr;
    SDL_LockSurface(conv);
    Uint8* px = (Uint8*)conv->pixels;
    for (int i = 0; i < conv->h; ++i) {
        for (int j = 0; j < conv->w; ++j) {
            Uint8* p = px + i * conv->pitch + j * 4;
            p[0] = 255 - p[0]; // R
            p[1] = 255 - p[1]; // G
            p[2] = 255 - p[2]; // B
        }
    }
    SDL_UnlockSurface(conv);
    return conv;
}
