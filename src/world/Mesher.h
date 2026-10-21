#pragma once

#include "core/Blocks.h"
#include "world/Chunk.h"
#include "world/ChunkStorage.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace voxel::world {

using BlockQueryFn = std::function<voxel::core::BlockId(int wx, int wy, int wz)>;

struct MeshQuad {
    int axis = 0; // 0=X, 1=Y, 2=Z
    bool positive = true;
    int slice = 0;
    int u = 0;
    int v = 0;
    int width = 1;
    int height = 1;
    voxel::core::BlockId blockId = 0;
};

struct MesherResult {
    size_t quadCount = 0;
    size_t coveredUnitFaces = 0;
    std::vector<MeshQuad> quads;
};

[[nodiscard]] MesherResult meshChunkGreedy(const ChunkPos& pos, const ChunkStorage& storage,
                                           BlockQueryFn blockAtWorld,
                                           const voxel::core::BlocksFile& blocks);

} // namespace voxel::world
