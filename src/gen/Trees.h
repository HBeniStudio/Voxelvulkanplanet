#pragma once

#include "gen/Biomes.h"
#include "world/Chunk.h"
#include "world/ChunkStorage.h"

#include <FastNoise/FastNoise.h>

#include <cstdint>

namespace voxel::gen {

void applyTrees(const WorldGenData& data, const FastNoise::SmartNode<>& biomeNoise, uint64_t seed,
                voxel::world::ChunkPos pos, voxel::world::ChunkStorage& storage);

} // namespace voxel::gen
