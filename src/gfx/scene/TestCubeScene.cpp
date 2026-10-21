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

    auto& center = scene.chunks[{0, 0, 0}];
    for (int z = 12; z < 20; ++z) {
        for (int y = 12; y < 20; ++y) {
            for (int x = 12; x < 20; ++x) {
                const size_t idx = static_cast<size_t>(x + y * kChunkSize + z * kChunkSize * kChunkSize);
                const int edge = (x == 12 || x == 19 || y == 12 || y == 19 || z == 12 || z == 19) ? 1 : 0;
                center[idx] = edge ? static_cast<voxel::core::BlockId>(3) : static_cast<voxel::core::BlockId>(1);
            }
        }
    }

    auto& east = scene.chunks[{1, 0, 0}];
    east[16 + 16 * kChunkSize + 16 * kChunkSize * kChunkSize] = 2;
    auto& north = scene.chunks[{0, 0, 1}];
    north[16 + 16 * kChunkSize + 16 * kChunkSize * kChunkSize] = 4;

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
