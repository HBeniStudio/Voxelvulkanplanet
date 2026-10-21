#include "gen/WorldBorder.h"

#include "world/ChunkStorage.h"

namespace voxel::gen {

namespace {

constexpr voxel::core::BlockId kBedrock = 10;

bool isOnRegionEdge(int wx, int wz, const RegionSize& region) {
    return wx <= 0 || wz <= 0 || wx >= region.size_x - 1 || wz >= region.size_z - 1;
}

bool isOutsideRegion(int wx, int wz, const RegionSize& region) {
    return wx < 0 || wz < 0 || wx >= region.size_x || wz >= region.size_z;
}

} // namespace

void applyWorldBorder(const WorldGenData& data, voxel::world::ChunkPos pos,
                      voxel::world::ChunkStorage& storage) {
    const int baseY = pos.y * voxel::world::kChunkSize;
    const int wx0 = pos.x * voxel::world::kChunkSize;
    const int wz0 = pos.z * voxel::world::kChunkSize;

    for (int lz = 0; lz < voxel::world::kChunkSize; ++lz) {
        for (int lx = 0; lx < voxel::world::kChunkSize; ++lx) {
            const int wx = wx0 + lx;
            const int wz = wz0 + lz;
            if (isOutsideRegion(wx, wz, data.region)) {
                for (int ly = 0; ly < voxel::world::kChunkSize; ++ly) {
                    storage.set(lx, ly, lz, 0);
                }
                continue;
            }
            if (!isOnRegionEdge(wx, wz, data.region)) {
                continue;
            }
            for (int ly = 0; ly < voxel::world::kChunkSize; ++ly) {
                const int wy = baseY + ly;
                if (wy < 0 || wy >= data.region.size_y) {
                    storage.set(lx, ly, lz, 0);
                    continue;
                }
                storage.set(lx, ly, lz, kBedrock);
            }
        }
    }
}

} // namespace voxel::gen
