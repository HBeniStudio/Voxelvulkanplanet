#include "core/World.h"

#include <stdexcept>

namespace voxel::core {

void World::loadBlocks(const std::filesystem::path& path) {
    blocks = loadBlocksJson(path);
    const std::string err = validateBlocksSchemaV1(blocks);
    if (!err.empty()) {
        throw std::runtime_error(err);
    }
}

} // namespace voxel::core
