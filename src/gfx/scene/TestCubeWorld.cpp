#include "gfx/scene/TestCubeScene.h"

#include "world/ChunkStorage.h"
#include "world/World.h"

#include <chrono>
#include <thread>
#include <utility>

namespace voxel::gfx::scene {

void populateWorldFromTestCube(voxel::world::World& world) {
    const TestCubeScene scene = TestCubeScene::build3x3x3Chunks();
    for (const auto& [key, voxels] : scene.chunks) {
        voxel::world::ChunkStorage storage;
        storage.fill(0);
        for (int z = 0; z < kChunkSize; ++z) {
            for (int y = 0; y < kChunkSize; ++y) {
                for (int x = 0; x < kChunkSize; ++x) {
                    const size_t idx =
                        static_cast<size_t>(x + y * kChunkSize + z * kChunkSize * kChunkSize);
                    storage.set(x, y, z, voxels[idx]);
                }
            }
        }
        world.chunkManager.setChunkStorage({key.x, key.y, key.z}, std::move(storage));
    }

    for (int i = 0; i < 500; ++i) {
        world.chunkManager.pumpCompletedMeshes();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

} // namespace voxel::gfx::scene
