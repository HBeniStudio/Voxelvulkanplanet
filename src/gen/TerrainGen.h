#pragma once

#include "gen/Biomes.h"
#include "world/IWorldGen.h"

#include <FastNoise/FastNoise.h>

#include <memory>

namespace voxel::gen {

class TerrainGen final : public voxel::world::IWorldGen {
public:
    explicit TerrainGen(WorldGenData data);

    void generate(voxel::world::ChunkPos pos, voxel::world::ChunkStorage& storage,
                  uint64_t seed) const override;

    [[nodiscard]] const WorldGenData& data() const noexcept { return data_; }

private:
    WorldGenData data_;
    FastNoise::SmartNode<> heightNoise_;
    FastNoise::SmartNode<> biomeNoise_;

    [[nodiscard]] int surfaceHeightAt(int wx, int wz, uint64_t seed) const;
};

[[nodiscard]] uint64_t hashChunkStorage(const voxel::world::ChunkStorage& storage);

} // namespace voxel::gen
