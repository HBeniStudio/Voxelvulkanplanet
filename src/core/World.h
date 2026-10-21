#pragma once

#include "core/Blocks.h"

#include <filesystem>

namespace voxel::core {

// Simulation state container; expanded in later phases.
struct World {
    BlocksFile blocks;

    void loadBlocks(const std::filesystem::path& path);
};

} // namespace voxel::core
