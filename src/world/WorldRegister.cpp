#include "core/Config.h"
#include "core/Registry.h"
#include "gen/Biomes.h"
#include "gen/TerrainGen.h"
#include "world/FlatWorldGen.h"
#include "world/World.h"

#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>

namespace {

std::unique_ptr<voxel::gen::TerrainGen> g_terrainGen;

class WorldChunkStartupSystem final : public voxel::core::ISystem {
public:
    void update(voxel::core::World& world, float) override {
        const voxel::core::Config& cfg = voxel::core::Config::active();
        if (cfg.scene == "terrain") {
            const std::filesystem::path blocksPath =
                std::filesystem::path(VOXEL_SOURCE_DIR) / "data/blocks.json";
            const std::filesystem::path worldGenPath =
                std::filesystem::path(VOXEL_SOURCE_DIR) / "data/biomes.json";
            world.loadBlocks(blocksPath);
            const voxel::gen::WorldGenData data = voxel::gen::loadWorldGenJson(worldGenPath);
            const std::string err = voxel::gen::validateWorldGenSchemaV1(data, world.blocks);
            if (!err.empty()) {
                throw std::runtime_error("worldgen validation failed: " + err);
            }
            g_terrainGen = std::make_unique<voxel::gen::TerrainGen>(data);
            world.chunkManager.setWorldGen(g_terrainGen.get(), cfg.worldSeed);
            spdlog::debug("WorldChunkStartupSystem: terrain generator registered (seed={})",
                          cfg.worldSeed);
            return;
        }

        static const voxel::world::FlatWorldGen kFlatGen;
        world.chunkManager.setWorldGen(&kFlatGen, 0);
        spdlog::debug("WorldChunkStartupSystem: flat test generator registered");
    }
};

REGISTER_SYSTEM(WorldChunkStartupSystem, voxel::core::Phase::Startup);

} // namespace
