#pragma once

#include "core/Blocks.h"
#include "world/Chunk.h"
#include "world/ChunkStorage.h"
#include "world/MeshVertex.h"
#include "world/Mesher.h"

#include <functional>
#include <string>
#include <vector>

namespace voxel::world {

using BlockQueryFn = std::function<voxel::core::BlockId(int wx, int wy, int wz)>;
using TextureLayerFn = std::function<float(const std::string& textureName)>;

[[nodiscard]] std::vector<MeshVertex> buildChunkMeshVertices(const ChunkPos& pos, const ChunkStorage& storage,
                                                             BlockQueryFn blockAtWorld,
                                                             const voxel::core::BlocksFile& blocks,
                                                             TextureLayerFn layerForTexture);

[[nodiscard]] std::vector<MeshVertex> buildVerticesFromQuads(const ChunkPos& pos,
                                                             const std::vector<MeshQuad>& quads,
                                                             const voxel::core::BlocksFile& blocks,
                                                             TextureLayerFn layerForTexture);

} // namespace voxel::world
