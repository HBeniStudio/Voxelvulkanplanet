#pragma once

#include "world/IWorldGen.h"

#include <cstdint>

namespace voxel::world {

// P3 test generator: flat stone layer at y=0 in world space, air elsewhere.
class FlatWorldGen final : public IWorldGen {
public:
    void generate(ChunkPos pos, ChunkStorage& storage, uint64_t seed) const override;
};

} // namespace voxel::world
