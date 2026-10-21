#pragma once

#include "core/Blocks.h"
#include "world/Chunk.h"
#include "world/ChunkStorage.h"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace voxel::world {

using BlockQueryFn = std::function<voxel::core::BlockId(int wx, int wy, int wz)>;

struct MesherResult {
    size_t quadCount = 0;
};

// Binary greedy meshing (layer bitmasks + 2D greedy merge). Neighbor query uses world block coords.
[[nodiscard]] MesherResult meshChunkGreedy(const ChunkPos& pos, const ChunkStorage& storage,
                                           BlockQueryFn blockAtWorld,
                                           const voxel::core::BlocksFile& blocks);

} // namespace voxel::world
