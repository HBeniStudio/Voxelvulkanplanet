#include "world/FlatWorldGen.h"

#include "world/Chunk.h"

namespace voxel::world {

void FlatWorldGen::generate(ChunkPos pos, ChunkStorage& storage, uint64_t /*seed*/) const {
    storage.fill(0);
    const int baseY = pos.y * kChunkSize;
    for (int lz = 0; lz < kChunkSize; ++lz) {
        for (int lx = 0; lx < kChunkSize; ++lx) {
            for (int ly = 0; ly < kChunkSize; ++ly) {
                const int wy = baseY + ly;
                if (wy == 0) {
                    storage.set(lx, ly, lz, 1);
                }
            }
        }
    }
}

} // namespace voxel::world
