#pragma once

#include "core/Blocks.h"
#include "world/ChunkManager.h"

#include <filesystem>

namespace voxel::world {

struct World {
    voxel::core::BlocksFile blocks;
    ChunkManager chunkManager;

    void loadBlocks(const std::filesystem::path& path);

    [[nodiscard]] voxel::core::BlockId getBlock(int wx, int wy, int wz) const;
    void setBlock(int wx, int wy, int wz, voxel::core::BlockId id);
};

} // namespace voxel::world
