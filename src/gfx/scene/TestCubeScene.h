#pragma once

#include "core/Blocks.h"

namespace voxel::world {
struct World;
}

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace voxel::gfx::scene {

struct ChunkPosKey {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;

    bool operator==(const ChunkPosKey& o) const { return x == o.x && y == o.y && z == o.z; }
};

struct ChunkPosHash {
    size_t operator()(const ChunkPosKey& p) const noexcept {
        const size_t h1 = std::hash<int32_t>{}(p.x);
        const size_t h2 = std::hash<int32_t>{}(p.y);
        const size_t h3 = std::hash<int32_t>{}(p.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

inline constexpr int kChunkSize = 32;

struct TestCubeScene {
    std::unordered_map<ChunkPosKey, std::vector<voxel::core::BlockId>, ChunkPosHash> chunks;

    static TestCubeScene build3x3x3Chunks();
    [[nodiscard]] voxel::core::BlockId blockAtWorld(int wx, int wy, int wz) const;
    [[nodiscard]] std::vector<ChunkPosKey> chunkPositions() const;
};

void populateWorldFromTestCube(voxel::world::World& world);

} // namespace voxel::gfx::scene
