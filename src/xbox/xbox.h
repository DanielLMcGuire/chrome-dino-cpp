#pragma once

#include <windows.h>
#include <cstdlib>
#include <cstdio>
#include "util.h"
#include <array>
#include <vector>
#include <optional>
#include <string>
#include <cstdint>

extern "C" {
    #include <xboxkrnl/xboxkrnl.h>
    #include <hal/video.h>
    #include <hal/debug.h>
    #include <hal/fileio.h>
}

#define HAL_LED_H // do not load led.h

#define XUnreachable() __builtin_unreachable()

namespace xbox {
    struct resolution_t {
        int height;
        int width;
    };

    // Namespace functions
    [[noreturn]] void reboot(bool quick = false);
    void logError(const char* what, const char* detail, bool crash = false);
    std::uint32_t getX86TimingSeed(std::uint32_t seed);
    std::optional<std::string> convertDOSFilename(const std::string &dosFilename);

    class Video {
    public:
        Video();
        Video(const resolution_t &maxResolution);
        Video(const int width, const int height);
        Video(const VIDEO_MODE &vm);
        Video(const resolution_t &maxResolution, const VIDEO_MODE &minimum_vm);
        ~Video();

        resolution_t getCurrentResolution();
        void initVideo(const resolution_t &maxResolution, const VIDEO_MODE &minimum_vm);
        void aliasSmoothing(bool enable);
        void interlaceFilter(int level);
        void enable(bool enable);
        bool enabled();

        static void waitForVBlank();
        static void set(unsigned char *fb);
        static void flush();
        static int memory();

    private:
        static VIDEO_MODE getHighestSupportedResolution(const VIDEO_MODE &minimum_vm, const resolution_t &maxResolution);
        
        bool isrRegistered = false;
        resolution_t resolution;
        bool softenSet = false;
        bool softenFilter = false;
        bool flickerSet = false;
        int flickerLevel = 5;
        bool encEnabled = true;
    };

    struct LED {
        enum Color {
            OFF = 0x00,
            GREEN = 0x01,
            RED = 0x10,
            ORANGE = 0x11
        };

        static constexpr ULONG MODE_AUTO = 0x00;
        static constexpr ULONG MODE_MANUAL = 0x01;
        static constexpr ULONG SMC_REG_LEDMODE = 0x07;
        static constexpr ULONG SMC_REG_LEDSEQ = 0x08;

        static void set(const std::array<Color, 4> &seq);
        static void reset();
    };
}

// C-API Layer
typedef enum _XLEDColor {
    XLED_OFF = 0x00,
    XLED_GREEN = 0x01,
    XLED_RED = 0x10,
    XLED_ORANGE = 0x11
} XLEDColor;

extern xbox::Video *__embedded_xbox_video_class;
extern bool _hasMadeVideoClass;

#ifndef XBOX_IMPL

#define XBOX_VIDEO_M_CLASS __embedded_xbox_video_class
#define BARE_CHECK_VIDEO_CLASS if (XBOX_VIDEO_M_CLASS)
#define BARE_CHECK_NO_VIDEO_CLASS if (!XBOX_VIDEO_M_CLASS)
#define CHECK_VIDEO_CLASS(func) \
    BARE_CHECK_NO_VIDEO_CLASS xbox::logError("You need to call XVideoSetMode!", func, true)
#define CHECKED_VIDEO_CLASS(func) CHECK_VIDEO_CLASS(func); else XBOX_VIDEO_M_CLASS
#define DELETE_VIDEO_CLASS() BARE_CHECK_VIDEO_CLASS delete XBOX_VIDEO_M_CLASS
#define CREATE_VIDEO_CLASS(x, y) BARE_CHECK_VIDEO_CLASS new xbox::Video(x, y);

#define BARE_CHECK_NO_VIDEO_CREATED if (!_hasMadeVideoClass)

#define XSetCustomLED(a, b, c, d) \
    xbox::LED::set({(xbox::LED::Color)a, (xbox::LED::Color)b, (xbox::LED::Color)c, (xbox::LED::Color)d})
#define XResetLED() xbox::LED::reset()

#define XVideoInit() \
    DELETE_VIDEO_CLASS(); \
    xbox::logError("Do not call XVideoInit! Use XVideoSetMode!", "XVideoInit", true)
#define XVideoSetMode(w, h, b, f) \
    BARE_CHECK_NO_VIDEO_CLASS XBOX_VIDEO_M_CLASS = new xbox::Video({w, h, b, f}); \
    else XBOX_VIDEO_M_CLASS->initVideo({w, h}, {w, h, b, f})
#define XVideoSetFlickerFilter(y) CHECKED_VIDEO_CLASS("XVideoSetFlickerFilter").interlaceFilter(y)
#define XVideoSetSoftenFilter(y) CHECKED_VIDEO_CLASS("XVideoSetSoftenFilter").aliasSmoothing(y)
#define XVideoSetFB(x) xbox::Video::set(x)
#define XVideoFlushFB(x) xbox::Video::flush()

#define XConvertDOSFilenameToXBOX(x, y) y = xbox::convertDOSFilename(x);
#define XReboot() DELETE_VIDEO_CLASS(); xbox::reboot()

// Bootstrap Layer
#define USER_MAIN xbx_main
#define __xbx_init main
extern int USER_MAIN(int argc, char *argv[]);

#ifndef XBOX_INIT
#define main USER_MAIN
#endif

#endif