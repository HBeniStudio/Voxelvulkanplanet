#include <doctest/doctest.h>

#include "world/ChunkStorage.h"
#include "world/RegionFile.h"

#include <filesystem>
#include <unordered_map>

TEST_CASE("RegionFile save and load round-trip") {
    const std::filesystem::path worldDir =
        std::filesystem::path(VOXEL_SOURCE_DIR) / "build/tests-out/region_test_world";
    std::filesystem::remove_all(worldDir);

    std::unordered_map<voxel::world::ChunkPos, voxel::world::ChunkStorage, voxel::world::ChunkPosHash> chunks;
    voxel::world::ChunkStorage a;
    a.set(1, 2, 3, 4);
    a.set(10, 11, 12, 5);
    chunks.emplace(voxel::world::ChunkPos{0, 0, 0}, a);

    voxel::world::ChunkStorage b;
    b.fill(2);
    b.set(0, 0, 0, 7);
    chunks.emplace(voxel::world::ChunkPos{5, 3, 7}, b);

    const voxel::world::RegionCoord region = voxel::world::regionOfChunk({0, 0, 0});
    voxel::world::RegionFile::saveRegion(worldDir, region, chunks);

    const auto loaded = voxel::world::RegionFile::loadRegion(worldDir, region);
    REQUIRE(loaded.size() == chunks.size());
    for (const auto& [pos, storage] : chunks) {
        REQUIRE(loaded.contains(pos));
        CHECK(loaded.at(pos) == storage);
    }
}
