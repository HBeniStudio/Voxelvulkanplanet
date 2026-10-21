#pragma once

#include "gen/Biomes.h"
#include "world/Chunk.h"
#include "world/ChunkStorage.h"

#include <cstdint>

namespace voxel::gen {

void applyOres(const WorldGenData& data, uint64_t seed, voxel::world::ChunkPos pos,
               voxel::world::ChunkStorage& storage);

} // namespace voxel::gen
