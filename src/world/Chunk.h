#pragma once

#include "world/ChunkStorage.h"

#include <cstdint>
#include <functional>

namespace voxel::world {

struct ChunkPos {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;

    bool operator==(const ChunkPos& o) const noexcept {
        return x == o.x && y == o.y && z == o.z;
    }
};

struct ChunkPosHash {
    size_t operator()(const ChunkPos& p) const noexcept {
        const size_t h1 = std::hash<int32_t>{}(p.x);
        const size_t h2 = std::hash<int32_t>{}(p.y);
        const size_t h3 = std::hash<int32_t>{}(p.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

class Chunk {
public:
    explicit Chunk(ChunkPos pos) : pos_(pos) {}

    [[nodiscard]] ChunkPos position() const noexcept { return pos_; }
    [[nodiscard]] ChunkStorage& storage() noexcept { return storage_; }
    [[nodiscard]] const ChunkStorage& storage() const noexcept { return storage_; }

private:
    ChunkPos pos_;
    ChunkStorage storage_;
};

} // namespace voxel::world
