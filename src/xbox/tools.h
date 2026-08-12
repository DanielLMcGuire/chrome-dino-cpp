#pragma once

#include <windows.h>
#include <cstdio>
#include "util.h"

extern "C" {
    #include <hal/led.h>
    #include <hal/video.h>
    #include <hal/xbox.h>
    #include <hal/debug.h>
}

namespace xboxhelper {

typedef struct {
    int height;
    int width;
} resolution_t;

VIDEO_MODE getHighestSupportedXboxResolution(void) {
    VIDEO_MODE vm;
    VIDEO_MODE highest = {640, 480, 32, REFRESH_DEFAULT}; 
    void *context = NULL;
    DWORD maxPixels = 0;

    while (XVideoListModes(&vm, 0, 0, &context)) {
        if (vm.width > 1280 || vm.height > 720) {
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

resolution_t initXboxVideo(void) {
    VIDEO_MODE res = getHighestSupportedXboxResolution();
    WINDOW_WIDTH = res.width;
    WINDOW_HEIGHT = res.height;
    XVideoSetMode(res.width, res.height, res.bpp, res.refresh);
    resolution_t ret = {res.height, res.width};
    return ret;
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
    XReboot();
    __builtin_unreachable();
}

static std::uint64_t ReadTSC()
{
    std::uint32_t lo;
    std::uint32_t hi;

    __asm__ __volatile__(
        "rdtsc"
        : "=a"(lo), "=d"(hi)
    );

    return (static_cast<std::uint64_t>(hi) << 32) | lo;
}

static std::uint32_t getX86TimingSeed(std::uint32_t seed)
{
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
}