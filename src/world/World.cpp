#include "world/World.h"

#include <filesystem>
#include <stdexcept>
#include <string>

namespace voxel::world {

void World::loadBlocks(const std::filesystem::path& path) {
    blocks = voxel::core::loadBlocksJson(path);
    const std::string err = voxel::core::validateBlocksSchemaV1(blocks);
    if (!err.empty()) {
        throw std::runtime_error(err);
    }
    chunkManager.setBlocks(&blocks);
}

voxel::core::BlockId World::getBlock(int wx, int wy, int wz) const {
    return chunkManager.getBlock(wx, wy, wz);
}

void World::setBlock(int wx, int wy, int wz, voxel::core::BlockId id) {
    chunkManager.setBlock(wx, wy, wz, id);
}

} // namespace voxel::world
