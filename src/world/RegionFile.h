#pragma once

#include "world/Chunk.h"
#include "world/ChunkStorage.h"

#include <cstdint>
#include <filesystem>
#include <unordered_map>

namespace voxel::world {

inline constexpr int kRegionSize = 16;

struct RegionCoord {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
};

[[nodiscard]] RegionCoord regionOfChunk(ChunkPos chunk);

class RegionFile {
public:
    static void saveRegion(const std::filesystem::path& worldDir, RegionCoord region,
                           const std::unordered_map<ChunkPos, ChunkStorage, ChunkPosHash>& chunks);
    [[nodiscard]] static std::unordered_map<ChunkPos, ChunkStorage, ChunkPosHash>
    loadRegion(const std::filesystem::path& worldDir, RegionCoord region);
};

} // namespace voxel::world
