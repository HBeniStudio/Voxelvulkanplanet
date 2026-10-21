#include <doctest/doctest.h>

#include "core/Blocks.h"
#include "world/Mesher.h"

#include <filesystem>
#include <stdexcept>

namespace {

voxel::core::BlocksFile loadBlocks() {
    const std::filesystem::path path = std::filesystem::path(VOXEL_SOURCE_DIR) / "data/blocks.json";
    voxel::core::BlocksFile file = voxel::core::loadBlocksJson(path);
    const std::string err = voxel::core::validateBlocksSchemaV1(file);
    if (!err.empty()) {
        throw std::runtime_error(err);
    }
    return file;
}

voxel::world::BlockQueryFn airOutside() {
    return [](int, int, int) -> voxel::core::BlockId { return 0; };
}

} // namespace

TEST_CASE("mesher quad counts for known shapes") {
    const voxel::core::BlocksFile blocks = loadBlocks();
    const voxel::world::ChunkPos pos{0, 0, 0};

    voxel::world::ChunkStorage single;
    single.fill(0);
    single.set(16, 16, 16, 1);
    const auto singleResult =
        voxel::world::meshChunkGreedy(pos, single, airOutside(), blocks);
    CHECK(singleResult.quadCount == 6);

    voxel::world::ChunkStorage solid;
    solid.fill(1);
    const auto solidResult = voxel::world::meshChunkGreedy(pos, solid, airOutside(), blocks);
    CHECK(solidResult.quadCount == 6);

    voxel::world::ChunkStorage checker;
    checker.fill(0);
    for (int z = 0; z < voxel::world::kChunkSize; ++z) {
        for (int y = 0; y < voxel::world::kChunkSize; ++y) {
            for (int x = 0; x < voxel::world::kChunkSize; ++x) {
                if (((x + y + z) & 1) == 0) {
                    checker.set(x, y, z, 1);
                }
            }
        }
    }
    const auto checkerResult = voxel::world::meshChunkGreedy(pos, checker, airOutside(), blocks);
    // Each solid cell is isolated (no same-type neighbors); greedy merge cannot combine faces.
    CHECK(checkerResult.quadCount == 98304);

    voxel::world::ChunkStorage lShape;
    lShape.fill(0);
    const int coords[][3] = {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {0, 0, 1}, {0, 0, 2}};
    for (const auto& c : coords) {
        lShape.set(c[0], c[1], c[2], 1);
    }
    const auto lResult = voxel::world::meshChunkGreedy(pos, lShape, airOutside(), blocks);
    CHECK(lResult.quadCount == 10);
}
