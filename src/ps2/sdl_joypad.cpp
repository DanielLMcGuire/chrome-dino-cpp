#include "sdl_joypad.h"

#include <string>

namespace ps2sdl {

namespace {
unsigned char g_pad_buffers[2][256] __attribute__((aligned(64)));
} // namespace

Sint16 Ps2StickToSdl(unsigned char value)
{
    int v = static_cast<int>(value);

    if (v >= 128) {
        return static_cast<Sint16>(((v - 128) * 32767) / 127);
    }

    return static_cast<Sint16>(-((128 - v) * 32768) / 128);
}

Sint16 Ps2TriggerToSdl(unsigned char pressure, bool digitalPressed)
{
    if (pressure != 0) {
        return static_cast<Sint16>((static_cast<int>(pressure) * 32767) / 255);
    }

    return digitalPressed ? 32767 : 0;
}

Ps2SdlPad::~Ps2SdlPad()
{
    shutdown();
}

bool Ps2SdlPad::init()
{
    if (!attachVirtualController()) {
        return false;
    }

    last_buttons_.fill(false);
    last_axes_.fill(0);

    pad_initialised_ = (padInit(0) == 1);

    for (int port = 0; port < 2; ++port) {
        pad_buf_[port] = g_pad_buffers[port];
        port_open_[port] = false;
        mode_set_[port] = false;
        press_set_[port] = false;
        dualshock_[port] = false;
        pressure_[port] = false;
    }

    next_open_try_ = 0;
    return true;
}

void Ps2SdlPad::shutdown()
{
    for (int port = 0; port < 2; ++port) {
        if (port_open_[port]) {
            padPortClose(port, 0);
            port_open_[port] = false;
        }

        pad_buf_[port] = nullptr;

        mode_set_[port] = false;
        press_set_[port] = false;
        dualshock_[port] = false;
        pressure_[port] = false;
    }

    if (pad_initialised_) {
        padEnd();
        pad_initialised_ = false;
    }

    detachVirtualController();
}

void Ps2SdlPad::update()
{
    int active = pollActivePort();

    if (active >= 0) {
        struct padButtonStatus status;
        if (padRead(active, 0, &status) != 0) {
            applyStatus(active, status);
        } else {
            applyNeutral();
        }
    } else {
        applyNeutral();
    }

    if (joy_ != nullptr) {
        SDL_JoystickUpdate();
    }
}

bool Ps2SdlPad::attachVirtualController()
{
    if (joy_ != nullptr || controller_ != nullptr) {
        return true;
    }

    device_index_ = SDL_JoystickAttachVirtual(
        SDL_JOYSTICK_TYPE_GAMECONTROLLER,
        SDL_CONTROLLER_AXIS_MAX,
        SDL_CONTROLLER_BUTTON_MAX,
        0
    );

    if (device_index_ < 0) {
        return false;
    }

    addMappingForDeviceIndex(device_index_);

    controller_ = SDL_GameControllerOpen(device_index_);
    if (controller_ != nullptr) {
        joy_ = SDL_GameControllerGetJoystick(controller_);
    } else {
        joy_ = SDL_JoystickOpen(device_index_);
    }

    if (joy_ == nullptr) {
        detachVirtualController();
        return false;
    }

    for (int b = 0; b < SDL_CONTROLLER_BUTTON_MAX; ++b) {
        SDL_JoystickSetVirtualButton(joy_, b, SDL_RELEASED);
    }

    for (int a = 0; a < SDL_CONTROLLER_AXIS_MAX; ++a) {
        SDL_JoystickSetVirtualAxis(joy_, a, 0);
    }

    last_buttons_.fill(false);
    last_axes_.fill(0);

    return true;
}

void Ps2SdlPad::detachVirtualController()
{
    if (controller_ != nullptr) {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
        joy_ = nullptr;
    } else if (joy_ != nullptr) {
        SDL_JoystickClose(joy_);
        joy_ = nullptr;
    }

    if (device_index_ >= 0) {
        SDL_JoystickDetachVirtual(device_index_);
        device_index_ = -1;
    }
}

bool Ps2SdlPad::addMappingForDeviceIndex(int device_index)
{
    SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(device_index);

    char guid_string[33] = {};
    SDL_JoystickGetGUIDString(guid, guid_string, sizeof(guid_string));

    std::string mapping;
    mapping += guid_string;
    mapping += ",PS2 DualShock2 Controller,platform:";
    mapping += SDL_GetPlatform();
    mapping +=
        ",a:b14,b:b13,x:b15,y:b12,back:b0,guide:b5,start:b3,leftstick:b1,rightstick:b2,"
        "leftshoulder:b10,rightshoulder:b11,dpup:b4,dpdown:b6,dpleft:b7,dpright:b5,"
        "lefttrigger:a4,righttrigger:a5,leftx:a0,lefty:a1,rightx:a2,righty:a3";

    return SDL_GameControllerAddMapping(mapping.c_str()) >= 0;
}

int Ps2SdlPad::pollActivePort()
{
    Uint32 now = SDL_GetTicks();

    if (now >= next_open_try_) {
        for (int port = 0; port < 2; ++port) {
            if (!port_open_[port] && pad_buf_[port] != nullptr) {
                if (padPortOpen(port, 0, pad_buf_[port]) != 0) {
                    port_open_[port] = true;
                    mode_set_[port] = false;
                    press_set_[port] = false;
                    dualshock_[port] = false;
                    pressure_[port] = false;
                }
            }
        }

        next_open_try_ = now + 250;
    }

    for (int port = 0; port < 2; ++port) {
        if (!port_open_[port]) {
            continue;
        }

        int state = padGetState(port, 0);

        if (state == PAD_STATE_DISCONN) {
            mode_set_[port] = false;
            press_set_[port] = false;
            dualshock_[port] = false;
            pressure_[port] = false;
            continue;
        }

        if (state != PAD_STATE_STABLE) {
            continue;
        }

        if (!mode_set_[port]) {
            configureMode(port);
            mode_set_[port] = true;
            continue;
        }

        if (!press_set_[port]) {
            configurePressure(port);
            press_set_[port] = true;
            continue;
        }

        return port;
    }

    return -1;
}

void Ps2SdlPad::configureMode(int port)
{
    dualshock_[port] = false;

    int modes = padInfoMode(port, 0, PAD_MODETABLE, -1);
    if (modes > 0) {
        for (int i = 0; i < modes; ++i) {
            if (padInfoMode(port, 0, PAD_MODETABLE, i) == PAD_TYPE_DUALSHOCK) {
                dualshock_[port] = true;
                break;
            }
        }
    }

    if (dualshock_[port]) {
        padSetMainMode(port, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
    } else {
        padSetMainMode(port, 0, PAD_MMODE_DIGITAL, PAD_MMODE_UNLOCK);
    }
}

void Ps2SdlPad::configurePressure(int port)
{
    pressure_[port] = false;

    if (dualshock_[port] && padInfoPressMode(port, 0) != 0) {
        if (padEnterPressMode(port, 0) != 0) {
            pressure_[port] = true;
        }
    }
}

void Ps2SdlPad::applyStatus(int port, const struct padButtonStatus& status)
{
    uint16_t mask = static_cast<uint16_t>(0xFFFF ^ status.btns);

    std::array<bool, SDL_CONTROLLER_BUTTON_MAX> buttons{};
    std::array<Sint16, SDL_CONTROLLER_AXIS_MAX> axes{};

    buttons[PS2SDL_SELECT]   = (mask & PAD_SELECT) != 0;
    buttons[PS2SDL_L3]       = (mask & PAD_L3) != 0;
    buttons[PS2SDL_R3]       = (mask & PAD_R3) != 0;
    buttons[PS2SDL_START]    = (mask & PAD_START) != 0;
    buttons[PS2SDL_UP]       = (mask & PAD_UP) != 0;
    buttons[PS2SDL_RIGHT]    = (mask & PAD_RIGHT) != 0;
    buttons[PS2SDL_DOWN]     = (mask & PAD_DOWN) != 0;
    buttons[PS2SDL_LEFT]     = (mask & PAD_LEFT) != 0;
    buttons[PS2SDL_L2]       = (mask & PAD_L2) != 0;
    buttons[PS2SDL_R2]       = (mask & PAD_R2) != 0;
    buttons[PS2SDL_L1]       = (mask & PAD_L1) != 0;
    buttons[PS2SDL_R1]       = (mask & PAD_R1) != 0;
    buttons[PS2SDL_TRIANGLE] = (mask & PAD_TRIANGLE) != 0;
    buttons[PS2SDL_CIRCLE]   = (mask & PAD_CIRCLE) != 0;
    buttons[PS2SDL_CROSS]    = (mask & PAD_CROSS) != 0;
    buttons[PS2SDL_SQUARE]   = (mask & PAD_SQUARE) != 0;

    if (dualshock_[port]) {
        axes[SDL_CONTROLLER_AXIS_LEFTX] = Ps2StickToSdl(status.ljoy_h);
        axes[SDL_CONTROLLER_AXIS_LEFTY] = Ps2StickToSdl(status.ljoy_v);
        axes[SDL_CONTROLLER_AXIS_RIGHTX] = Ps2StickToSdl(status.rjoy_h);
        axes[SDL_CONTROLLER_AXIS_RIGHTY] = Ps2StickToSdl(status.rjoy_v);
    } else {
        axes[SDL_CONTROLLER_AXIS_LEFTX] = 0;
        axes[SDL_CONTROLLER_AXIS_LEFTY] = 0;
        axes[SDL_CONTROLLER_AXIS_RIGHTX] = 0;
        axes[SDL_CONTROLLER_AXIS_RIGHTY] = 0;
    }

    axes[SDL_CONTROLLER_AXIS_TRIGGERLEFT] =
        Ps2TriggerToSdl(status.l2_p, (mask & PAD_L2) != 0);
    axes[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] =
        Ps2TriggerToSdl(status.r2_p, (mask & PAD_R2) != 0);

    applyArrays(buttons, axes);
}

void Ps2SdlPad::applyNeutral()
{
    std::array<bool, SDL_CONTROLLER_BUTTON_MAX> buttons{};
    std::array<Sint16, SDL_CONTROLLER_AXIS_MAX> axes{};

    applyArrays(buttons, axes);
}

void Ps2SdlPad::applyArrays(
    const std::array<bool, SDL_CONTROLLER_BUTTON_MAX>& buttons,
    const std::array<Sint16, SDL_CONTROLLER_AXIS_MAX>& axes
)
{
    if (joy_ == nullptr) {
        return;
    }

    for (int b = 0; b < SDL_CONTROLLER_BUTTON_MAX; ++b) {
        if (buttons[b] != last_buttons_[b]) {
            SDL_JoystickSetVirtualButton(
                joy_,
                b,
                buttons[b] ? SDL_PRESSED : SDL_RELEASED
            );
        }
    }

    for (int a = 0; a < SDL_CONTROLLER_AXIS_MAX; ++a) {
        if (axes[a] != last_axes_[a]) {
            SDL_JoystickSetVirtualAxis(joy_, a, axes[a]);
        }
    }

    last_buttons_ = buttons;
    last_axes_ = axes;
}

} // namespace ps2sdl