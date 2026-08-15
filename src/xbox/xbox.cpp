#define XBOX_IMPL
#include "xbox.h"

xbox::Video *__embedded_xbox_video_class = nullptr;

namespace {
    static std::uint64_t ReadTSC() {
        std::uint32_t lo;
        std::uint32_t hi;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
        return (static_cast<std::uint64_t>(hi) << 32) | lo;
    }
}

void xbox::reboot(bool quick) {
    HalReturnToFirmware(quick ? HalQuickRebootRoutine : HalRebootRoutine);
    XUnreachable();
}

void xbox::logError(const char* what, const char* detail, bool crash) {
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

std::uint32_t xbox::getX86TimingSeed(std::uint32_t seed) {
    std::uint32_t state = seed;
    std::uint64_t last = ReadTSC();

    for (int i = 0; i < 64; ++i) {
        volatile std::uint32_t x = state;
        for (int j = 0; j < 32 + (state & 31); ++j) {
            x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        }
        const std::uint64_t now = ReadTSC();
        const std::uint64_t delta = now - last;
        last = now;
        state ^= static_cast<std::uint32_t>(delta);
        state ^= static_cast<std::uint32_t>(delta >> 32);
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        state += 0x9E3779B9u + static_cast<std::uint32_t>(x);
    }
    return seedMix(state);
}

std::optional<std::string> xbox::convertDOSFilename(const std::string &dosFilename) {
    char buffer[MAX_PATH];
    if (XConvertDOSFilenameToXBOX(dosFilename.c_str(), buffer) != STATUS_SUCCESS)
        return std::nullopt;
    return buffer;
}