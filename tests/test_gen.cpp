#include <doctest/doctest.h>

#include "core/Blocks.h"
#include "gen/Biomes.h"
#include "gen/TerrainGen.h"
#include "world/Chunk.h"

#include <filesystem>
#include <future>
#include <string>

namespace {

voxel::gen::WorldGenData loadValidatedWorldGen() {
    const std::filesystem::path blocksPath =
        std::filesystem::path(VOXEL_SOURCE_DIR) / "data/blocks.json";
    const std::filesystem::path worldGenPath =
        std::filesystem::path(VOXEL_SOURCE_DIR) / "data/biomes.json";
    const voxel::core::BlocksFile blocks = voxel::core::loadBlocksJson(blocksPath);
    REQUIRE(voxel::core::validateBlocksSchemaV1(blocks).empty());

    voxel::gen::WorldGenData data = voxel::gen::loadWorldGenJson(worldGenPath);
    REQUIRE(voxel::gen::validateWorldGenSchemaV1(data, blocks).empty());
    return data;
}

uint64_t generateHash(const voxel::gen::TerrainGen& gen, voxel::world::ChunkPos pos, uint64_t seed) {
    voxel::world::ChunkStorage storage;
    gen.generate(pos, storage, seed);
    return voxel::gen::hashChunkStorage(storage);
}

} // namespace

TEST_CASE("worldgen schema v1 accepts shipped biomes.json") {
    const auto data = loadValidatedWorldGen();
    CHECK(data.schema == 1);
    CHECK(data.biomes.size() == 4);
    CHECK(data.ores.size() == 2);
    CHECK(data.trees.size() == 1);
}

TEST_CASE("worldgen schema v1 rejects invalid block ids and fields") {
    const voxel::core::BlocksFile blocks = voxel::core::loadBlocksJson(
        std::filesystem::path(VOXEL_SOURCE_DIR) / "data/blocks.json");

    voxel::gen::WorldGenData good = loadValidatedWorldGen();

    voxel::gen::WorldGenData badSchema = good;
    badSchema.schema = 2;
    CHECK_FALSE(voxel::gen::validateWorldGenSchemaV1(badSchema, blocks).empty());

    voxel::gen::WorldGenData badBlock = good;
    badBlock.biomes[0].surface_block = 99;
    const std::string blockErr = voxel::gen::validateWorldGenSchemaV1(badBlock, blocks);
    CHECK_FALSE(blockErr.empty());
    CHECK(blockErr.find("unknown block") != std::string::npos);

    voxel::gen::WorldGenData badOre = good;
    badOre.ores[0].min_y = 200;
    badOre.ores[0].max_y = 10;
    CHECK_FALSE(voxel::gen::validateWorldGenSchemaV1(badOre, blocks).empty());
}

TEST_CASE("terrain generation is seed-deterministic across runs and threads") {
    const voxel::gen::WorldGenData data = loadValidatedWorldGen();
    const voxel::gen::TerrainGen gen(data);
    const voxel::world::ChunkPos pos{2, 1, -3};
    constexpr uint64_t seed = 0xDEADBEEF42ULL;

    const uint64_t hashA = generateHash(gen, pos, seed);
    const uint64_t hashB = generateHash(gen, pos, seed);
    CHECK(hashA == hashB);

    const uint64_t hashOtherSeed = generateHash(gen, pos, seed + 1);
    CHECK(hashA != hashOtherSeed);

    auto fut1 = std::async(std::launch::async, [&] {
        const voxel::gen::TerrainGen localGen(data);
        return generateHash(localGen, pos, seed);
    });
    auto fut2 = std::async(std::launch::async, [&] {
        const voxel::gen::TerrainGen localGen(data);
        return generateHash(localGen, pos, seed);
    });
    const uint64_t hashThreadA = fut1.get();
    const uint64_t hashThreadB = fut2.get();
    CHECK(hashThreadA == hashThreadB);
    CHECK(hashThreadA == hashA);
}

TEST_CASE("terrain places bedrock at floor and water at sea level below surface") {
    const voxel::gen::WorldGenData data = loadValidatedWorldGen();
    const voxel::gen::TerrainGen gen(data);
    constexpr uint64_t seed = 42;

    voxel::world::ChunkStorage floorChunk;
    gen.generate({0, 0, 0}, floorChunk, seed);
    for (int x = 0; x < voxel::world::kChunkSize; ++x) {
        for (int z = 0; z < voxel::world::kChunkSize; ++z) {
            CHECK(floorChunk.get(x, 0, z) == 10);
        }
    }

    std::array<voxel::world::ChunkStorage, 4> columns{};
    for (int chunkY = 0; chunkY < 4; ++chunkY) {
        gen.generate({0, chunkY, 0}, columns[static_cast<size_t>(chunkY)], seed);
    }

    const auto worldBlock = [&](int wx, int wy, int wz) -> voxel::core::BlockId {
        const int chunkY = wy / voxel::world::kChunkSize;
        if (chunkY < 0 || chunkY >= 4 || wx < 0 || wx >= voxel::world::kChunkSize || wz < 0 ||
            wz >= voxel::world::kChunkSize) {
            return 0;
        }
        const int ly = wy - chunkY * voxel::world::kChunkSize;
        return columns[static_cast<size_t>(chunkY)].get(wx, ly, wz);
    };

    bool verifiedWaterColumn = false;
    for (int x = 0; x < voxel::world::kChunkSize; ++x) {
        for (int z = 0; z < voxel::world::kChunkSize; ++z) {
            int surfaceWorldY = -1;
            for (int wy = 0; wy <= data.sea_level; ++wy) {
                const auto id = worldBlock(x, wy, z);
                if (id != 0 && id != 5) {
                    surfaceWorldY = wy;
                }
            }
            if (surfaceWorldY < 0 || surfaceWorldY >= data.sea_level) {
                continue;
            }
            verifiedWaterColumn = true;
            for (int wy = surfaceWorldY + 1; wy <= data.sea_level; ++wy) {
                CHECK(worldBlock(x, wy, z) == 5);
            }
        }
    }
    CHECK(verifiedWaterColumn);
}
