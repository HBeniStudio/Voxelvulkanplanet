#include <doctest/doctest.h>

#include "world/ChunkStorage.h"

#include <set>

TEST_CASE("ChunkStorage uniform fill and get/set") {
    voxel::world::ChunkStorage storage;
    storage.fill(3);
    CHECK(storage.isUniform());
    CHECK(storage.get(0, 0, 0) == 3);
    CHECK(storage.get(31, 31, 31) == 3);

    storage.set(5, 6, 7, 1);
    CHECK_FALSE(storage.isUniform());
    CHECK(storage.get(5, 6, 7) == 1);
    CHECK(storage.get(0, 0, 0) == 3);
}

TEST_CASE("ChunkStorage serialize round-trip") {
    voxel::world::ChunkStorage storage;
    storage.set(1, 2, 3, 4);
    storage.set(10, 11, 12, 5);
    const std::vector<uint8_t> bytes = storage.serialize();
    const voxel::world::ChunkStorage restored = voxel::world::ChunkStorage::deserialize(bytes);
    CHECK(storage == restored);
}

TEST_CASE("ChunkStorage palette grows past 16 and 256 entries") {
    voxel::world::ChunkStorage storage;
    storage.fill(0);

    for (int i = 0; i < 17; ++i) {
        storage.set(i, 0, 0, static_cast<voxel::core::BlockId>(i + 1));
    }
    CHECK(storage.palette().size() >= 17);
    CHECK(storage.bitsPerIndex() >= 5);

    for (int i = 0; i < 17; ++i) {
        CHECK(storage.get(i, 0, 0) == static_cast<voxel::core::BlockId>(i + 1));
    }

    std::set<voxel::core::BlockId> ids;
    for (int i = 0; i < 257; ++i) {
        const voxel::core::BlockId id = static_cast<voxel::core::BlockId>(1000 + i);
        storage.set(i % 32, (i / 32) % 32, i / (32 * 32), id);
        ids.insert(id);
    }
    CHECK(storage.palette().size() >= 257);
    CHECK(storage.bitsPerIndex() >= 9);

    const std::vector<uint8_t> bytes = storage.serialize();
    const voxel::world::ChunkStorage restored = voxel::world::ChunkStorage::deserialize(bytes);
    CHECK(storage == restored);
}
