#pragma once

#include "world/Chunk.h"
#include "world/ChunkStorage.h"

#include <cstdint>

namespace voxel::world {

class IWorldGen {
public:
    virtual ~IWorldGen() = default;
    virtual void generate(ChunkPos pos, ChunkStorage& storage, uint64_t seed) const = 0;
};

} // namespace voxel::world
