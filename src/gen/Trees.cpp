#include "gen/Trees.h"

#include "gen/Biomes.h"
#include "gen/HashUtil.h"
#include "world/ChunkStorage.h"

#include <algorithm>
#include <cmath>

namespace voxel::gen {

namespace {

constexpr voxel::core::BlockId kBedrock = 10;

bool isSolidSurface(voxel::core::BlockId id) {
    return id != 0 && id != 5;
}

} // namespace

void applyTrees(const WorldGenData& data, const FastNoise::SmartNode<>& biomeNoise, uint64_t seed,
                voxel::world::ChunkPos pos, voxel::world::ChunkStorage& storage) {
    if (data.trees.empty()) {
        return;
    }
    const TreeDef& tree = data.trees[0];

    const int baseY = pos.y * voxel::world::kChunkSize;
    const int wx0 = pos.x * voxel::world::kChunkSize;
    const int wz0 = pos.z * voxel::world::kChunkSize;

    for (int lz = 0; lz < voxel::world::kChunkSize; ++lz) {
        for (int lx = 0; lx < voxel::world::kChunkSize; ++lx) {
            const int wx = wx0 + lx;
            const int wz = wz0 + lz;

            int surfaceLy = -1;
            for (int ly = voxel::world::kChunkSize - 1; ly >= 0; --ly) {
                const auto id = storage.get(lx, ly, lz);
                if (isSolidSurface(id)) {
                    surfaceLy = ly;
                    break;
                }
            }
            if (surfaceLy < 0) {
                continue;
            }

            const int biomeSeed = static_cast<int>(seed) + data.biome_noise.seed_offset;
            const float posX = static_cast<float>(wx);
            const float posZ = static_cast<float>(wz);
            float biomeSample = 0.0f;
            biomeNoise->GenPositionArray2D(&biomeSample, 1, &posX, &posZ, 0.0f, 0.0f, biomeSeed);
            const BiomeDef& biome = pickBiome(data, biomeSample);
            if (biome.tree_density <= 0.0f) {
                continue;
            }

            const int surfaceWy = baseY + surfaceLy;
            const float roll = hashToUnit(seed, wx, surfaceWy, wz);
            if (roll > biome.tree_density) {
                continue;
            }

            const float heightRoll = hashToUnit(seed + 17, wx, surfaceWy, wz);
            const int trunkHeight = tree.min_height + static_cast<int>(std::floor(
                heightRoll * static_cast<float>(tree.max_height - tree.min_height + 1)));
            const int clampedHeight = std::clamp(trunkHeight, tree.min_height, tree.max_height);

            for (int h = 1; h <= clampedHeight; ++h) {
                const int ly = surfaceLy + h;
                if (ly >= voxel::world::kChunkSize) {
                    break;
                }
                storage.set(lx, ly, lz, tree.log_block);
            }

            const int canopyBaseLy = surfaceLy + clampedHeight - 1;
            for (int dz = -tree.canopy_radius; dz <= tree.canopy_radius; ++dz) {
                for (int dy = -1; dy <= tree.canopy_radius; ++dy) {
                    for (int dx = -tree.canopy_radius; dx <= tree.canopy_radius; ++dx) {
                        if (dx * dx + dy * dy + dz * dz > tree.canopy_radius * tree.canopy_radius + 1) {
                            continue;
                        }
                        const int lx2 = lx + dx;
                        const int ly2 = canopyBaseLy + dy;
                        const int lz2 = lz + dz;
                        if (lx2 < 0 || lx2 >= voxel::world::kChunkSize || ly2 < 0 ||
                            ly2 >= voxel::world::kChunkSize || lz2 < 0 ||
                            lz2 >= voxel::world::kChunkSize) {
                            continue;
                        }
                        if (storage.get(lx2, ly2, lz2) == 0) {
                            storage.set(lx2, ly2, lz2, tree.leaves_block);
                        }
                    }
                }
            }
        }
    }
}

} // namespace voxel::gen
