#pragma once

#include <cstdlib>
#include <ctime>

inline float randFloat() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); // NOLINT
}

inline int randInt(int lo, int hi) {
    if (lo >= hi) return lo;
    return lo + std::rand() % (hi - lo + 1); // NOLINT
}


static std::uint32_t getSeed() {
    const std::uint32_t timeSeed =
    static_cast<std::uint32_t>(std::time(nullptr));

    const std::uint32_t timerSeed =
        static_cast<std::uint32_t>(SDL_GetTicks());

    const std::uint32_t addressSeed =
        static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(&timeSeed)
        );

    static std::uint32_t seed = timeSeed;

    seed ^= timerSeed + 0x9e3779b9u + (seed << 6) + (seed >> 2);
    seed ^= addressSeed + 0x9e3779b9u + (seed << 6) + (seed >> 2);

    seed ^= seed >> 16;
    seed *= 0x85ebca6bu;
    seed ^= seed >> 13;
    seed *= 0xc2b2ae35u;
    seed ^= seed >> 16;

    return seed;
}

#if __PS2__
    #define GET_RAND_SEED(x) x.getIOPTimingSeed(getSeed())
#else
    #define GET_RAND_SEED() getSeed()
#endif