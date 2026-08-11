#pragma once

#include <array>
#include <cstdint>

#include <SDL2/SDL.h>

#if !SDL_VERSION_ATLEAST(2, 0, 14)
#error This file requires SDL 2.0.14 or newer for SDL_JoystickAttachVirtual().
#endif

extern "C" {
#include <libpad.h>
}

namespace ps2sdl {

enum : int {
    PS2SDL_PAD_SELECT   = 0,
    PS2SDL_PAD_L3       = 1,
    PS2SDL_PAD_R3       = 2,
    PS2SDL_PAD_START    = 3,
    PS2SDL_PAD_UP       = 4,
    PS2SDL_PAD_RIGHT    = 5,
    PS2SDL_PAD_DOWN     = 6,
    PS2SDL_PAD_LEFT     = 7,
    PS2SDL_PAD_L2       = 8,
    PS2SDL_PAD_R2       = 9,
    PS2SDL_PAD_L1       = 10,
    PS2SDL_PAD_R1       = 11,
    PS2SDL_PAD_TRIANGLE = 12,
    PS2SDL_PAD_CIRCLE   = 13,
    PS2SDL_PAD_CROSS    = 14,
    PS2SDL_PAD_SQUARE   = 15,
};

enum : int {
    PS2SDL_CONTROLLER_BUTTON_CROSS    = SDL_CONTROLLER_BUTTON_A,
    PS2SDL_CONTROLLER_BUTTON_CIRCLE   = SDL_CONTROLLER_BUTTON_B,
    PS2SDL_CONTROLLER_BUTTON_SQUARE   = SDL_CONTROLLER_BUTTON_X,
    PS2SDL_CONTROLLER_BUTTON_TRIANGLE = SDL_CONTROLLER_BUTTON_Y,
    PS2SDL_CONTROLLER_BUTTON_SELECT   = SDL_CONTROLLER_BUTTON_BACK,
    PS2SDL_CONTROLLER_BUTTON_START    = SDL_CONTROLLER_BUTTON_START,
    PS2SDL_CONTROLLER_BUTTON_L3       = SDL_CONTROLLER_BUTTON_LEFTSTICK,
    PS2SDL_CONTROLLER_BUTTON_R3       = SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    PS2SDL_CONTROLLER_BUTTON_L1       = SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    PS2SDL_CONTROLLER_BUTTON_R1       = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    PS2SDL_CONTROLLER_BUTTON_UP       = SDL_CONTROLLER_BUTTON_DPAD_UP,
    PS2SDL_CONTROLLER_BUTTON_DOWN     = SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    PS2SDL_CONTROLLER_BUTTON_LEFT     = SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    PS2SDL_CONTROLLER_BUTTON_RIGHT    = SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
};

Sint16 Ps2StickToSdl(unsigned char value);

Sint16 Ps2TriggerToSdl(unsigned char pressure, bool digitalPressed);

class Ps2SdlPad {
public:
    Ps2SdlPad() = default;
    ~Ps2SdlPad();

    Ps2SdlPad(const Ps2SdlPad&) = delete;
    Ps2SdlPad& operator=(const Ps2SdlPad&) = delete;

    std::uint32_t getIOPTimingSeed(std::uint32_t seed);
    bool init();
    void shutdown();
    void update();

private:
    SDL_GameController* controller_ = nullptr;
    SDL_Joystick* joy_ = nullptr;
    int device_index_ = -1;

    bool pad_initialised_ = false;

    void* pad_buf_[2] = {nullptr, nullptr};
    bool port_open_[2] = {false, false};
    bool mode_set_[2] = {false, false};
    bool press_set_[2] = {false, false};
    bool dualshock_[2] = {false, false};
    bool pressure_[2] = {false, false};

    Uint32 next_open_try_ = 0;

    std::array<bool, SDL_CONTROLLER_BUTTON_MAX> last_buttons_{};
    std::array<Sint16, SDL_CONTROLLER_AXIS_MAX> last_axes_{};

    bool attachVirtualController();
    void detachVirtualController();
    bool addMappingForDeviceIndex(int device_index);
    int pollActivePort();
    void configureMode(int port);
    void configurePressure(int port);
    void applyStatus(int port, const struct padButtonStatus& status);
    void applyNeutral();
    void applyArrays(
        const std::array<bool, SDL_CONTROLLER_BUTTON_MAX>& buttons,
        const std::array<Sint16, SDL_CONTROLLER_AXIS_MAX>& axes
    );
    std::uint32_t mixIOPTimingSeed(std::uint32_t seed, std::uint32_t elapsed, const padButtonStatus& status, unsigned char result);
};

} // namespace ps2sdl
