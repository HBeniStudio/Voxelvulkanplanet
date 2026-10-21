#include "gfx/scene/TerrainScene.h"

#include "core/Config.h"
#include "core/Registry.h"
#include "gfx/TerrainCamera.h"
#include "world/WorldSession.h"

#include <filesystem>

namespace voxel::gfx::scene {

void setupTerrainWorld(voxel::world::World& world) {
    const std::filesystem::path blocksPath =
        std::filesystem::path(VOXEL_SOURCE_DIR) / "data/blocks.json";
    world.loadBlocks(blocksPath);
    voxel::core::Registry::instance().runPhase(voxel::core::Phase::Startup, world, 0.0f);

    voxel::world::WorldSession::setActive(&world);
    const voxel::gfx::Camera camera = makeTerrainGoldenCamera();
    voxel::world::WorldSession::setStreamingPosition(camera.position);

    const voxel::world::ChunkPos center =
        voxel::world::chunkPosFromWorldPosition(camera.position);
    world.chunkManager.updateAround(center, voxel::core::Config::active().streamHorizontalDistance,
                                    voxel::core::Config::active().streamVerticalDistance);
    world.chunkManager.waitForPendingMeshes();
}

} // namespace voxel::gfx::scene
