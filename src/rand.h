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

static std::uint32_t seedMix(std::uint32_t seed) {
    seed ^= seed >> 16;
    seed *= 0x85ebca6bu;
    seed ^= seed >> 13;
    seed *= 0xc2b2ae35u;
    seed ^= seed >> 16;
    return seed;
}

static std::uint32_t getTimeAddressInitSeed() {
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

    return seedMix(seed);
}

#if __XBOX__
static std::uint32_t getX86TimingSeed(std::uint32_t seed)
{
    auto ReadTSC = []() -> std::uint64_t
    {
        std::uint32_t lo;
        std::uint32_t hi;

        __asm__ __volatile__(
            "rdtsc"
            : "=a"(lo), "=d"(hi)
        );

        return (static_cast<std::uint64_t>(hi) << 32) | lo;
    };

    std::uint32_t state = seed;

    for (int i = 0; i < 32; ++i)
    {
        const std::uint64_t tsc = ReadTSC();

        const std::uint32_t lo = static_cast<std::uint32_t>(tsc);
        const std::uint32_t hi = static_cast<std::uint32_t>(tsc >> 32);

        state ^= lo + 0x9E3779B9u + (state << 6) + (state >> 2);
        state ^= hi + 0x85EBCA6Bu + (state << 7) + (state >> 3);

    }

    return seedMix(state);
}
#endif

#if __PS2__
    #define GET_RAND_SEED(x) x.getIOPTimingSeed(getTimeAddressInitSeed())
#elif __XBOX__
    #define GET_RAND_SEED() getX86TimingSeed(getTimeAddressInitSeed())
#else
    #define GET_RAND_SEED() getTimeAddressInitSeed()
#endif