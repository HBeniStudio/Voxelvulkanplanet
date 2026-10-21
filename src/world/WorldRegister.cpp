#include "core/Registry.h"
#include "world/FlatWorldGen.h"
#include "world/World.h"

#include <spdlog/spdlog.h>

namespace {

class WorldChunkStartupSystem final : public voxel::core::ISystem {
public:
    void update(voxel::core::World& world, float) override {
        static bool once = false;
        if (once) {
            return;
        }
        once = true;
        static const voxel::world::FlatWorldGen kFlatGen;
        world.chunkManager.setWorldGen(&kFlatGen, 0);
        spdlog::debug("WorldChunkStartupSystem: flat test generator registered");
    }
};

REGISTER_SYSTEM(WorldChunkStartupSystem, voxel::core::Phase::Startup);

} // namespace
