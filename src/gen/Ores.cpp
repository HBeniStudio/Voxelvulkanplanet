#include "gen/Ores.h"

#include "world/ChunkStorage.h"

#include <algorithm>
#include <vector>

namespace voxel::gen {

namespace {

constexpr voxel::core::BlockId kBedrock = 10;

bool isStoneLike(voxel::core::BlockId id) {
    return id == 1;
}

} // namespace

void applyOres(const WorldGenData& data, const FastNoise::SmartNode<>& oreNoise, uint64_t seed,
               voxel::world::ChunkPos pos, voxel::world::ChunkStorage& storage) {
    const int baseY = pos.y * voxel::world::kChunkSize;
    const int wx0 = pos.x * voxel::world::kChunkSize;
    const int wz0 = pos.z * voxel::world::kChunkSize;

    const int volume = voxel::world::kChunkSize * voxel::world::kChunkSize * voxel::world::kChunkSize;
    std::vector<float> samples(static_cast<size_t>(volume));

    for (const OreDef& ore : data.ores) {
        const int oreSeed = static_cast<int>(seed) + static_cast<int>(ore.block) * 131;
        oreNoise->GenUniformGrid3D(samples.data(), static_cast<float>(wx0), static_cast<float>(baseY),
                                   static_cast<float>(wz0), voxel::world::kChunkSize,
                                   voxel::world::kChunkSize, voxel::world::kChunkSize, 1.0f, 1.0f,
                                   1.0f, oreSeed);

        const float threshold = 1.0f - std::min(ore.vein_frequency * ore.vein_size * 0.01f, 0.35f);

        int index = 0;
        for (int lz = 0; lz < voxel::world::kChunkSize; ++lz) {
            for (int ly = 0; ly < voxel::world::kChunkSize; ++ly) {
                for (int lx = 0; lx < voxel::world::kChunkSize; ++lx) {
                    const float sample = samples[static_cast<size_t>(index++)];
                    const int wy = baseY + ly;
                    if (wy < ore.min_y || wy > ore.max_y) {
                        continue;
                    }
                    const auto id = storage.get(lx, ly, lz);
                    if (!isStoneLike(id)) {
                        continue;
                    }
                    if (sample >= threshold) {
                        storage.set(lx, ly, lz, ore.block);
                    }
                }
            }
        }
    }
}

} // namespace voxel::gen
