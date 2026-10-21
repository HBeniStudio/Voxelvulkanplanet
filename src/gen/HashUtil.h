#pragma once

#include <cstdint>

namespace voxel::gen {

inline uint64_t mixSeed(uint64_t seed, int a, int b = 0, int c = 0) {
    uint64_t h = seed;
    h ^= static_cast<uint64_t>(a) * 0x9E3779B97F4A7C15ULL;
    h ^= static_cast<uint64_t>(b) * 0xBF58476D1CE4E5B9ULL;
    h ^= static_cast<uint64_t>(c) * 0x94D049BB133111EBULL;
    h ^= h >> 30;
    h *= 0xBF58476D1CE4E5B9ULL;
    h ^= h >> 27;
    h *= 0x94D049BB133111EBULL;
    h ^= h >> 31;
    return h;
}

inline float hashToUnit(uint64_t seed, int x, int y, int z) {
    const uint64_t h = mixSeed(seed, x, y, z);
    return static_cast<float>(h & 0xFFFFFF) / static_cast<float>(0xFFFFFF);
}

} // namespace voxel::gen
