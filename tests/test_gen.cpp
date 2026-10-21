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
    CHECK(data.biomes.size() >= 1);
    CHECK(data.ores.size() >= 1);
    CHECK(data.trees.size() >= 1);

    auto hasBiome = [&](const char* name) {
        for (const voxel::gen::BiomeDef& biome : data.biomes) {
            if (biome.name == name) {
                return true;
            }
        }
        return false;
    };
    CHECK(hasBiome("plains"));
    CHECK(hasBiome("forest"));
    CHECK(hasBiome("desert"));
    CHECK(hasBiome("mountains"));
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

    voxel::gen::WorldGenData badBiomeTree = good;
    badBiomeTree.biomes[0].tree_ids = {999};
    const std::string treeErr = voxel::gen::validateWorldGenSchemaV1(badBiomeTree, blocks);
    CHECK_FALSE(treeErr.empty());
    CHECK(treeErr.find("unknown tree id") != std::string::npos);

    voxel::gen::WorldGenData noOres = good;
    noOres.ores.clear();
    CHECK_FALSE(voxel::gen::validateWorldGenSchemaV1(noOres, blocks).empty());
}

TEST_CASE("terrain generation is seed-deterministic across runs and threads") {
    const voxel::gen::WorldGenData data = loadValidatedWorldGen();
    const voxel::gen::TerrainGen gen(data);
    const voxel::world::ChunkPos pos{8, 2, 8};
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

    int foundWx = -1;
    int foundWz = -1;
    int surfaceWorldY = -1;

    for (int wz = 64; wz < 256 && foundWx < 0; wz += 8) {
        for (int wx = 64; wx < 256; ++wx) {
            const int chunkX = wx / voxel::world::kChunkSize;
            const int chunkZ = wz / voxel::world::kChunkSize;
            std::array<voxel::world::ChunkStorage, 4> columns{};
            for (int chunkY = 0; chunkY < 4; ++chunkY) {
                gen.generate({chunkX, chunkY, chunkZ}, columns[static_cast<size_t>(chunkY)], seed);
            }
            const auto worldBlock = [&](int worldX, int worldY, int worldZ) -> voxel::core::BlockId {
                const int cy = worldY / voxel::world::kChunkSize;
                if (cy < 0 || cy >= 4) {
                    return 0;
                }
                const int cx = worldX / voxel::world::kChunkSize;
                const int cz = worldZ / voxel::world::kChunkSize;
                if (cx != chunkX || cz != chunkZ) {
                    return 0;
                }
                const int ly = worldY - cy * voxel::world::kChunkSize;
                return columns[static_cast<size_t>(cy)].get(worldX % voxel::world::kChunkSize, ly,
                                                           worldZ % voxel::world::kChunkSize);
            };

            int localSurface = -1;
            for (int wy = data.sea_level; wy >= 0; --wy) {
                const auto id = worldBlock(wx, wy, wz);
                if (id != 0 && id != 5) {
                    localSurface = wy;
                    break;
                }
            }
            if (localSurface < 0 || localSurface >= data.sea_level) {
                continue;
            }
            foundWx = wx;
            foundWz = wz;
            surfaceWorldY = localSurface;
            for (int wy = surfaceWorldY + 1; wy <= data.sea_level; ++wy) {
                CHECK(worldBlock(wx, wy, wz) == 5);
            }
            break;
        }
    }
    REQUIRE(foundWx >= 0);
}

TEST_CASE("ores appear only inside configured depth bands") {
    const voxel::gen::WorldGenData data = loadValidatedWorldGen();
    const voxel::gen::TerrainGen gen(data);
    constexpr uint64_t seed = 42;

    bool foundOre = false;
    for (int chunkY = 0; chunkY < 6; ++chunkY) {
        for (int chunkX = 0; chunkX < 4; ++chunkX) {
            voxel::world::ChunkStorage storage;
            gen.generate({chunkX, chunkY, 0}, storage, seed);
            const int baseY = chunkY * voxel::world::kChunkSize;
            for (int lz = 0; lz < voxel::world::kChunkSize; ++lz) {
                for (int ly = 0; ly < voxel::world::kChunkSize; ++ly) {
                    for (int lx = 0; lx < voxel::world::kChunkSize; ++lx) {
                        const auto id = storage.get(lx, ly, lz);
                        if (id != 8 && id != 9) {
                            continue;
                        }
                        foundOre = true;
                        const int wy = baseY + ly;
                        bool inBand = false;
                        for (const voxel::gen::OreDef& ore : data.ores) {
                            if (ore.block == id && wy >= ore.min_y && wy <= ore.max_y) {
                                inBand = true;
                                break;
                            }
                        }
                        CHECK(inBand);
                    }
                }
            }
        }
    }
    CHECK(foundOre);
}

TEST_CASE("trees use log and leaves blocks from data") {
    const voxel::gen::WorldGenData data = loadValidatedWorldGen();
    const voxel::gen::TerrainGen gen(data);
    constexpr uint64_t seed = 42;
    REQUIRE_FALSE(data.trees.empty());

    bool foundLog = false;
    bool foundLeaves = false;
    for (int chunkX = 0; chunkX < 8; ++chunkX) {
        for (int chunkZ = 0; chunkZ < 8; ++chunkZ) {
            voxel::world::ChunkStorage storage;
            gen.generate({chunkX, 2, chunkZ}, storage, seed);
            for (int lz = 0; lz < voxel::world::kChunkSize; ++lz) {
                for (int ly = 0; ly < voxel::world::kChunkSize; ++ly) {
                    for (int lx = 0; lx < voxel::world::kChunkSize; ++lx) {
                        const auto id = storage.get(lx, ly, lz);
                        if (id == data.trees[0].log_block) {
                            foundLog = true;
                        }
                        if (id == data.trees[0].leaves_block) {
                            foundLeaves = true;
                        }
                    }
                }
            }
        }
    }
    CHECK(foundLog);
    CHECK(foundLeaves);
}

TEST_CASE("world border is bedrock along region edges") {
    const voxel::gen::WorldGenData data = loadValidatedWorldGen();
    const voxel::gen::TerrainGen gen(data);
    constexpr uint64_t seed = 7;

    voxel::world::ChunkStorage edgeChunk;
    gen.generate({0, 2, 0}, edgeChunk, seed);
    const int edgeX = 0;
    const int edgeZ = 0;
    for (int ly = 0; ly < voxel::world::kChunkSize; ++ly) {
        const int wy = 2 * voxel::world::kChunkSize + ly;
        if (wy < 0 || wy >= data.region.size_y) {
            continue;
        }
        CHECK(edgeChunk.get(edgeX, ly, edgeZ) == 10);
    }

    const int maxChunkX = (data.region.size_x - 1) / voxel::world::kChunkSize;
    const int maxChunkZ = (data.region.size_z - 1) / voxel::world::kChunkSize;
    voxel::world::ChunkStorage farEdge;
    gen.generate({maxChunkX, 2, maxChunkZ}, farEdge, seed);
    const int lx = (data.region.size_x - 1) % voxel::world::kChunkSize;
    const int lz = (data.region.size_z - 1) % voxel::world::kChunkSize;
    for (int ly = 0; ly < voxel::world::kChunkSize; ++ly) {
        const int wy = 2 * voxel::world::kChunkSize + ly;
        if (wy < 0 || wy >= data.region.size_y) {
            continue;
        }
        CHECK(farEdge.get(lx, ly, lz) == 10);
    }
}

TEST_CASE("biome_noise maps noise samples to four biomes") {
    const voxel::gen::WorldGenData data = loadValidatedWorldGen();
    CHECK(voxel::gen::pickBiome(data, -0.75f).name == "plains");
    CHECK(voxel::gen::pickBiome(data, -0.25f).name == "forest");
    CHECK(voxel::gen::pickBiome(data, 0.25f).name == "desert");
    CHECK(voxel::gen::pickBiome(data, 0.75f).name == "mountains");
}
