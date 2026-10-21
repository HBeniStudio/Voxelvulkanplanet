#pragma once

#include "gen/Biomes.h"
#include "world/Chunk.h"
#include "world/ChunkStorage.h"

namespace voxel::gen {

void applyWorldBorder(const WorldGenData& data, voxel::world::ChunkPos pos,
                      voxel::world::ChunkStorage& storage);

} // namespace voxel::gen
