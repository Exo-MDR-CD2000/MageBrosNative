// The benchmark's command-line selectors, kept free of SDL so they can be unit-tested.
#pragma once
#include <cstdio>

namespace BenchmarkSpec {

// "w,l,x,z" for a campaign fragment (1-based world and level, the hero's x and z).
inline bool ParseFragment(const char* text, int* world, int* level, float* x, float* z) {
    return std::sscanf(text, "%d,%d,%f,%f", world, level, x, z) == 4;
}

// "w,l" for a level sweep.
inline bool ParseSweep(const char* text, int* world, int* level) {
    return std::sscanf(text, "%d,%d", world, level) == 2;
}

}  // namespace BenchmarkSpec
