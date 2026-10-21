#include "gen/Caves.h"

#include "world/ChunkStorage.h"

#include <vector>

namespace voxel::gen {

namespace {

constexpr voxel::core::BlockId kBedrock = 10;
constexpr voxel::core::BlockId kWater = 5;

bool isCarvable(voxel::core::BlockId id) {
    return id != 0 && id != kBedrock && id != kWater;
}

} // namespace

void applyCaves(const WorldGenData& data, const FastNoise::SmartNode<>& caveNoise, uint64_t seed,
                voxel::world::ChunkPos pos, voxel::world::ChunkStorage& storage) {
    const int baseY = pos.y * voxel::world::kChunkSize;
    const int wx0 = pos.x * voxel::world::kChunkSize;
    const int wz0 = pos.z * voxel::world::kChunkSize;
    const int caveSeed = static_cast<int>(seed) + data.cave_noise.seed_offset;

    const int volume = voxel::world::kChunkSize * voxel::world::kChunkSize * voxel::world::kChunkSize;
    std::vector<float> samples(static_cast<size_t>(volume));

    caveNoise->GenUniformGrid3D(samples.data(), static_cast<float>(wx0), static_cast<float>(baseY),
                                static_cast<float>(wz0), voxel::world::kChunkSize,
                                voxel::world::kChunkSize, voxel::world::kChunkSize, 1.0f, 1.0f, 1.0f,
                                caveSeed);

    constexpr float kCaveThreshold = 0.42f;
    int index = 0;
    for (int lz = 0; lz < voxel::world::kChunkSize; ++lz) {
        for (int ly = 0; ly < voxel::world::kChunkSize; ++ly) {
            for (int lx = 0; lx < voxel::world::kChunkSize; ++lx) {
                const float sample = samples[static_cast<size_t>(index++)];
                if (sample <= kCaveThreshold) {
                    continue;
                }
                const auto id = storage.get(lx, ly, lz);
                if (isCarvable(id)) {
                    storage.set(lx, ly, lz, 0);
                }
            }
        }
    }
}

} // namespace voxel::gen
