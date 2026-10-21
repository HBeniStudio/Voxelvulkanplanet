#include "gfx/scene/TestCubeScene.h"

namespace voxel::gfx::scene {

TestCubeScene TestCubeScene::build3x3x3Chunks() {
    TestCubeScene scene;
    for (int cz = -1; cz <= 1; ++cz) {
        for (int cy = -1; cy <= 1; ++cy) {
            for (int cx = -1; cx <= 1; ++cx) {
                std::vector<voxel::core::BlockId> v(static_cast<size_t>(kChunkSize * kChunkSize * kChunkSize),
                                                      0);
                scene.chunks[{cx, cy, cz}] = std::move(v);
            }
        }
    }

    for (auto& [pos, voxels] : scene.chunks) {
        const int index = (pos.x + 1) + (pos.y + 1) * 3 + (pos.z + 1) * 9;
        const voxel::core::BlockId shell = static_cast<voxel::core::BlockId>(1 + (index % 4));
        const voxel::core::BlockId core = static_cast<voxel::core::BlockId>(2 + (index % 3));
        for (int z = 13; z < 19; ++z) {
            for (int y = 13; y < 19; ++y) {
                for (int x = 13; x < 19; ++x) {
                    const size_t idx =
                        static_cast<size_t>(x + y * kChunkSize + z * kChunkSize * kChunkSize);
                    const bool edge = x == 13 || x == 18 || y == 13 || y == 18 || z == 13 || z == 18;
                    voxels[idx] = edge ? shell : core;
                }
            }
        }
    }

    return scene;
}

voxel::core::BlockId TestCubeScene::blockAtWorld(int wx, int wy, int wz) const {
    const int cx = wx >= 0 ? wx / kChunkSize : (wx - kChunkSize + 1) / kChunkSize;
    const int cy = wy >= 0 ? wy / kChunkSize : (wy - kChunkSize + 1) / kChunkSize;
    const int cz = wz >= 0 ? wz / kChunkSize : (wz - kChunkSize + 1) / kChunkSize;
    const auto it = chunks.find({cx, cy, cz});
    if (it == chunks.end()) {
        return 0;
    }
    const int lx = wx - cx * kChunkSize;
    const int ly = wy - cy * kChunkSize;
    const int lz = wz - cz * kChunkSize;
    if (lx < 0 || ly < 0 || lz < 0 || lx >= kChunkSize || ly >= kChunkSize || lz >= kChunkSize) {
        return 0;
    }
    const size_t idx = static_cast<size_t>(lx + ly * kChunkSize + lz * kChunkSize * kChunkSize);
    return it->second[idx];
}

std::vector<ChunkPosKey> TestCubeScene::chunkPositions() const {
    std::vector<ChunkPosKey> out;
    out.reserve(chunks.size());
    for (const auto& [pos, _] : chunks) {
        out.push_back(pos);
    }
    return out;
}

} // namespace voxel::gfx::scene
