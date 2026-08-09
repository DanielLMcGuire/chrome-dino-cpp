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
    PS2SDL_SELECT   = 0,
    PS2SDL_L3       = 1,
    PS2SDL_R3       = 2,
    PS2SDL_START    = 3,
    PS2SDL_UP       = 4,
    PS2SDL_RIGHT    = 5,
    PS2SDL_DOWN     = 6,
    PS2SDL_LEFT     = 7,
    PS2SDL_L2       = 8,
    PS2SDL_R2       = 9,
    PS2SDL_L1       = 10,
    PS2SDL_R1       = 11,
    PS2SDL_TRIANGLE = 12,
    PS2SDL_CIRCLE   = 13,
    PS2SDL_CROSS    = 14,
    PS2SDL_SQUARE   = 15,
};

Sint16 Ps2StickToSdl(unsigned char value);

Sint16 Ps2TriggerToSdl(unsigned char pressure, bool digitalPressed);

class Ps2SdlPad {
public:
    Ps2SdlPad() = default;
    ~Ps2SdlPad();

    Ps2SdlPad(const Ps2SdlPad&) = delete;
    Ps2SdlPad& operator=(const Ps2SdlPad&) = delete;

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
};

} // namespace ps2sdl