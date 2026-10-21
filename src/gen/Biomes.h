#pragma once

#include "core/Blocks.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace voxel::gen {

struct NoiseParams {
    float frequency = 0.004f;
    int octaves = 4;
    int seed_offset = 0;
};

struct RegionSize {
    int size_x = 4096;
    int size_y = 512;
    int size_z = 4096;
};

struct BiomeDef {
    int id = 0;
    std::string name;
    voxel::core::BlockId surface_block = 0;
    voxel::core::BlockId subsurface_block = 0;
    voxel::core::BlockId filler_block = 0;
    int height_base = 64;
    float height_amplitude = 8.0f;
    float tree_density = 0.0f;
    float noise_frequency = 1.0f;
    // Optional (schema v1.1): when non-empty, only these tree archetype ids may spawn here.
    std::vector<int> tree_ids;
};

struct OreDef {
    voxel::core::BlockId block = 0;
    std::string name;
    int min_y = 0;
    int max_y = 0;
    float vein_frequency = 0.02f;
    float vein_size = 8.0f;
};

struct TreeDef {
    int id = 0;
    voxel::core::BlockId log_block = 0;
    voxel::core::BlockId leaves_block = 0;
    int min_height = 4;
    int max_height = 7;
    int canopy_radius = 2;
};

struct WorldGenData {
    int schema = 0;
    int sea_level = 64;
    int bedrock_layers = 1;
    RegionSize region{};
    NoiseParams height_noise{};
    NoiseParams biome_noise{};
    NoiseParams cave_noise{};
    std::vector<BiomeDef> biomes;
    std::vector<OreDef> ores;
    std::vector<TreeDef> trees;
};

[[nodiscard]] WorldGenData loadWorldGenJson(const std::filesystem::path& path);
[[nodiscard]] std::string validateWorldGenSchemaV1(const WorldGenData& data,
                                                   const voxel::core::BlocksFile& blocks);

[[nodiscard]] const BiomeDef& pickBiome(const WorldGenData& data, float biomeNoiseSample);
[[nodiscard]] const TreeDef* findTreeById(const WorldGenData& data, int treeId);

} // namespace voxel::gen
