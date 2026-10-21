#include "core/Config.h"
#include "core/Registry.h"
#include "world/ChunkManager.h"
#include "world/WorldSession.h"

namespace {

class ChunkStreamingSystem final : public voxel::core::ISystem {
public:
    void update(voxel::core::World& fallbackWorld, float) override {
        const voxel::core::Config& cfg = voxel::core::Config::active();
        if (cfg.scene != "terrain") {
            return;
        }
        voxel::world::World* world = voxel::world::WorldSession::active();
        if (world == nullptr) {
            world = &fallbackWorld;
        }
        if (!voxel::world::WorldSession::hasStreamingPosition()) {
            return;
        }
        const voxel::world::ChunkPos center =
            voxel::world::chunkPosFromWorldPosition(voxel::world::WorldSession::streamingPosition());
        world->chunkManager.updateAround(center, cfg.streamHorizontalDistance, cfg.streamVerticalDistance);
    }
};

REGISTER_SYSTEM(ChunkStreamingSystem, voxel::core::Phase::Update);

} // namespace
