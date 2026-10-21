#pragma once

#include "gen/Biomes.h"
#include "world/Chunk.h"
#include "world/ChunkStorage.h"

#include <cstdint>

namespace voxel::gen {

// Carves air pockets from 3D cave noise (implemented in P4b).
void applyCaves(const WorldGenData& data, uint64_t seed, voxel::world::ChunkPos pos,
                voxel::world::ChunkStorage& storage);

} // namespace voxel::gen
