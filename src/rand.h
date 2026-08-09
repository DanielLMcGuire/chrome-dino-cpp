#pragma once

#include <cstdlib>

inline float randFloat() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); // NOLINT
}

inline int randInt(int lo, int hi) {
    if (lo >= hi) return lo;
    return lo + std::rand() % (hi - lo + 1); // NOLINT
}