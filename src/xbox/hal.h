#pragma once

#include <windows.h>
#include <cstdio>
#include "util.h"
#include <array>
#include <optional>

extern "C" {
    #include <xboxkrnl/xboxkrnl.h>
    #include <hal/video.h>
    #include <hal/debug.h>
    #include <hal/fileio.h>
}

#define XUnreachable() __builtin_unreachable()

namespace {
static std::uint64_t ReadTSC() {
    std::uint32_t lo;
    std::uint32_t hi;

    __asm__ __volatile__(
        "rdtsc"
        : "=a"(lo), "=d"(hi)
    );

    return (static_cast<std::uint64_t>(hi) << 32) | lo;
}
}

namespace xbox {

struct resolution_t {
    int height;
    int width;
};

class Video {
public:
    Video() {
        initVideo({1080, 1920}, {640, 480, 32, REFRESH_DEFAULT});
    }
    Video(const resolution_t &maxResolution) {
        initVideo(maxResolution, {640, 480, 32, REFRESH_DEFAULT});
    }
    Video(const int width, const int height) {
        initVideo({height, width}, {640, 480, 32, REFRESH_DEFAULT});
    }
    Video(const VIDEO_MODE &minimum_vm) {
        initVideo({1080, 1920}, minimum_vm);
    }
    Video(const resolution_t &maxResolution, const VIDEO_MODE &minimum_vm) {
        initVideo(maxResolution, minimum_vm);
    }
    ~Video() = default;

    inline resolution_t getCurrentResolution() { return resolution; }

    inline void initVideo(const resolution_t &maxResolution, const VIDEO_MODE &minimum_vm) {
        VIDEO_MODE res = getHighestSupportedResolution(minimum_vm, maxResolution);
        XVideoSetMode(res.width, res.height, res.bpp, res.refresh);
        resolution = {res.height, res.width};
    }

    inline void aliasSmoothing(bool enable) {
        if(!softenSet || softenFilter != enable) {
            AvSendTVEncoderOption((PVOID)VIDEO_BASE, VIDEO_ENC_SOFTEN_FILTER, (int)enable, nullptr);

            softenSet = true;
            softenFilter = enable;
        }
    }

    inline void interlaceFilter(int level) {
        if(!flickerSet || (level & 0x07) != flickerLevel)
        {
            AvSendTVEncoderOption((PVOID)VIDEO_BASE, VIDEO_ENC_FLICKERFILTER, (level & 0x07), nullptr);

            flickerSet = true;
            flickerLevel = level & 0x07;
        }
    }

    inline static void waitForVBlank() {
        XVideoWaitForVBlank();
    }

    inline static void set(unsigned char *fb) {
        XVideoSetFB(fb);
    }

    inline static void flush() {
        asm __volatile__("sfence");
    }

    inline void enable(bool enable) {
        AvSendTVEncoderOption((PVOID)VIDEO_BASE, VIDEO_ENC_VIDEOENABLE, !enable, nullptr);
    }

    inline static int memory() { return 1024 * 1024 * 4; }

private:
    inline static VIDEO_MODE getHighestSupportedResolution(const VIDEO_MODE &minimum_vm, const resolution_t &maxResolution) {
        VIDEO_MODE vm;
        VIDEO_MODE highest = minimum_vm; 
        void *context = nullptr;
        DWORD maxPixels = 0;

        while (XVideoListModes(&vm, 0, 0, &context)) {
            if (vm.width > maxResolution.width || vm.height > maxResolution.height) {
                continue;
            }

            DWORD currentPixels = (DWORD)vm.width * vm.height;
            if (currentPixels > maxPixels) {
                maxPixels = currentPixels;
                highest = vm;
            }
        }

        return highest;
    }
    bool isrRegistered = false;
    resolution_t resolution;
    bool softenSet = false;
    bool softenFilter = false;
    bool flickerSet = false;
    int flickerLevel = 5;
};

namespace LED {
enum Color {
    OFF = 0x00,
    GREEN = 0x01,
    RED = 0x10,
    ORANGE = 0x11
};

constexpr ULONG MODE_AUTO = 0x00;
constexpr ULONG MODE_MANUAL = 0x01;

constexpr ULONG SMC_REG_LEDMODE = 0x07;
constexpr ULONG SMC_REG_LEDSEQ = 0x08;

void set(const std::array<Color, 4> &seq) {
    HalWriteSMBusValue(0x20, SMC_REG_LEDMODE, FALSE, MODE_MANUAL);
    ULONG colorScheme =
        ((seq[0] & 0x11) << 3) |
        ((seq[1] & 0x11) << 2) |
        ((seq[2] & 0x11) << 1) |
        ((seq[3] & 0x11) << 0);
    HalWriteSMBusValue(0x20, SMC_REG_LEDSEQ, FALSE, colorScheme);
}

void reset() {
    HalWriteSMBusValue(0x20, SMC_REG_LEDMODE, FALSE, MODE_AUTO);
}
}

[[noreturn]] void reboot() {
    HalReturnToFirmware(HalRebootRoutine);
    XUnreachable();
}

void logError(const char* what, const char* detail, bool crash = false) {
#ifdef _DEBUG
    std::fprintf(stderr, "%s: %s", what, detail);
    constexpr DWORD time = 12 * 1000;
#else
    constexpr DWORD time = 5 * 1000;
#endif
    debugPrint("%s: %s\n", what, detail);
    if (!crash) return;
    Sleep(time);
    reboot();
    XUnreachable();
}

static std::uint32_t getX86TimingSeed(std::uint32_t seed) {
    std::uint32_t state = seed;

    std::uint64_t last = ReadTSC();

    for (int i = 0; i < 64; ++i)
    {
        volatile std::uint32_t x = state;

        for (int j = 0; j < 32 + (state & 31); ++j)
        {
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
        }

        const std::uint64_t now = ReadTSC();
        const std::uint64_t delta = now - last;

        last = now;

        state ^= static_cast<std::uint32_t>(delta);
        state ^= static_cast<std::uint32_t>(delta >> 32);

        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;

        state += 0x9E3779B9u + static_cast<std::uint32_t>(x);
    }

    return seedMix(state);
}

std::optional<std::string> convertDOSFilename(const std::string &dosFilename) {
    char buffer[512];
    if (XConvertDOSFilenameToXBOX(dosFilename.c_str(), buffer) != STATUS_SUCCESS)
        return std::nullopt;
    return buffer;
}
}

typedef enum _XLEDColor {
    XLED_OFF = 0x00,
    XLED_GREEN = 0x01,
    XLED_RED = 0x10,
    XLED_ORANGE = 0x11
} XLEDColor;

#define XSetCustomLED(a, b, c, x) xbox::LED::set({(xbox::LED::Color)a, (xbox::LED::Color)b, (xbox::LED::Color)c, (xbox::LED::Color)x})
#define XResetLED() xbox::LED::reset()

#define XVideoSetFlickerFilter(x, y) x.interlaceFilter(y)
#define XVideoSetSoftenFilter(x, y) x.aliasSmoothing(y)
#define XVideoSetFB(x) xbox::Video::set(x)
#define XVideoFlushFB(x) xbox::Video::flush(x)

#define XConvertDOSFilenameToXBOX(x, y) y = xbox::convertDOSFilename(x);

#define XReboot() xbox::reboot()