#include "gen/TerrainGen.h"

#include "gen/Caves.h"
#include "gen/Ores.h"
#include "gen/Trees.h"
#include "world/ChunkStorage.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace voxel::gen {

namespace {

constexpr voxel::core::BlockId kBedrock = 10;
constexpr voxel::core::BlockId kWater = 5;

FastNoise::SmartNode<> makeFractalNoise(const NoiseParams& params) {
    auto simplex = FastNoise::New<FastNoise::Simplex>();
    simplex->SetScale(1.0f / std::max(params.frequency, 1.0e-6f));
    auto fractal = FastNoise::New<FastNoise::FractalFBm>();
    fractal->SetSource(simplex);
    fractal->SetOctaveCount(static_cast<int>(std::max(1, params.octaves)));
    return fractal;
}

} // namespace

TerrainGen::TerrainGen(WorldGenData data) : data_(std::move(data)) {
    heightNoise_ = makeFractalNoise(data_.height_noise);
    biomeNoise_ = makeFractalNoise(data_.biome_noise);
}

int TerrainGen::surfaceHeightAt(int wx, int wz, uint64_t seed) const {
    const int heightSeed = static_cast<int>(seed) + data_.height_noise.seed_offset;
    const int biomeSeed = static_cast<int>(seed) + data_.biome_noise.seed_offset;

    const float posX = static_cast<float>(wx);
    const float posZ = static_cast<float>(wz);

    float biomeSample = 0.0f;
    biomeNoise_->GenPositionArray2D(&biomeSample, 1, &posX, &posZ, 0.0f, 0.0f, biomeSeed);

    const BiomeDef& biome = pickBiome(data_, biomeSample);

    float heightSample = 0.0f;
    heightNoise_->GenPositionArray2D(&heightSample, 1, &posX, &posZ, 0.0f, 0.0f, heightSeed);

    const float scaled =
        heightSample * biome.height_amplitude * biome.noise_frequency;
    const int height = biome.height_base + static_cast<int>(std::lround(scaled));
    return std::clamp(height, data_.bedrock_layers, data_.region.size_y - 1);
}

void TerrainGen::generate(voxel::world::ChunkPos pos, voxel::world::ChunkStorage& storage,
                          uint64_t seed) const {
    storage.fill(0);

    const int baseY = pos.y * voxel::world::kChunkSize;
    const int wx0 = pos.x * voxel::world::kChunkSize;
    const int wz0 = pos.z * voxel::world::kChunkSize;

    std::array<int, voxel::world::kChunkSize * voxel::world::kChunkSize> surfaceHeights{};
    for (int lz = 0; lz < voxel::world::kChunkSize; ++lz) {
        for (int lx = 0; lx < voxel::world::kChunkSize; ++lx) {
            const int wx = wx0 + lx;
            const int wz = wz0 + lz;
            surfaceHeights[static_cast<size_t>(lz * voxel::world::kChunkSize + lx)] =
                surfaceHeightAt(wx, wz, seed);
        }
    }

    for (int lz = 0; lz < voxel::world::kChunkSize; ++lz) {
        for (int lx = 0; lx < voxel::world::kChunkSize; ++lx) {
            const int wx = wx0 + lx;
            const int wz = wz0 + lz;
            const int surfaceY =
                surfaceHeights[static_cast<size_t>(lz * voxel::world::kChunkSize + lx)];

            const int biomeSeed = static_cast<int>(seed) + data_.biome_noise.seed_offset;
            const float posX = static_cast<float>(wx);
            const float posZ = static_cast<float>(wz);
            float biomeSample = 0.0f;
            biomeNoise_->GenPositionArray2D(&biomeSample, 1, &posX, &posZ, 0.0f, 0.0f, biomeSeed);
            const BiomeDef& biome = pickBiome(data_, biomeSample);

            for (int ly = 0; ly < voxel::world::kChunkSize; ++ly) {
                const int wy = baseY + ly;
                voxel::core::BlockId id = 0;

                if (wy < data_.bedrock_layers) {
                    id = kBedrock;
                } else if (wy <= surfaceY) {
                    const int depthBelowSurface = surfaceY - wy;
                    if (depthBelowSurface == 0) {
                        id = biome.surface_block;
                    } else if (depthBelowSurface <= 3) {
                        id = biome.subsurface_block;
                    } else {
                        id = biome.filler_block;
                    }
                } else if (wy <= data_.sea_level && surfaceY < data_.sea_level) {
                    id = kWater;
                }

                if (id != 0) {
                    storage.set(lx, ly, lz, id);
                }
            }
        }
    }

    applyCaves(data_, seed, pos, storage);
    applyOres(data_, seed, pos, storage);
    applyTrees(data_, seed, pos, storage);
}

uint64_t hashChunkStorage(const voxel::world::ChunkStorage& storage) {
    uint64_t hash = 14695981039346656037ULL;
    for (int z = 0; z < voxel::world::kChunkSize; ++z) {
        for (int y = 0; y < voxel::world::kChunkSize; ++y) {
            for (int x = 0; x < voxel::world::kChunkSize; ++x) {
                const auto id = static_cast<uint64_t>(storage.get(x, y, z));
                hash ^= id;
                hash *= 1099511628211ULL;
            }
        }
    }
    return hash;
}

} // namespace voxel::gen
