#include "gen/Biomes.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <unordered_set>

namespace voxel::gen {

namespace {

using json = nlohmann::json;

NoiseParams parseNoise(const json& node) {
    NoiseParams p;
    p.frequency = node.at("frequency").get<float>();
    p.octaves = node.at("octaves").get<int>();
    p.seed_offset = node.at("seed_offset").get<int>();
    return p;
}

bool isKnownBlockId(const voxel::core::BlocksFile& blocks, voxel::core::BlockId id) {
    if (id >= blocks.blocks.size()) {
        return false;
    }
    return blocks.blocks[id].id == id;
}

std::string checkBlockId(const char* context, voxel::core::BlockId id,
                         const voxel::core::BlocksFile& blocks) {
    if (!isKnownBlockId(blocks, id)) {
        return std::string(context) + ": unknown block id " + std::to_string(id);
    }
    return {};
}

} // namespace

WorldGenData loadWorldGenJson(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("failed to open worldgen file: " + path.string());
    }
    const json root = json::parse(in);

    WorldGenData data;
    data.schema = root.at("schema").get<int>();
    data.sea_level = root.at("sea_level").get<int>();
    data.bedrock_layers = root.at("bedrock_layers").get<int>();

    const auto& region = root.at("region");
    data.region.size_x = region.at("size_x").get<int>();
    data.region.size_y = region.at("size_y").get<int>();
    data.region.size_z = region.at("size_z").get<int>();

    data.height_noise = parseNoise(root.at("height_noise"));
    data.biome_noise = parseNoise(root.at("biome_noise"));
    data.cave_noise = parseNoise(root.at("cave_noise"));

    for (const auto& entry : root.at("biomes")) {
        BiomeDef biome;
        biome.id = entry.at("id").get<int>();
        biome.name = entry.at("name").get<std::string>();
        biome.surface_block = static_cast<voxel::core::BlockId>(entry.at("surface_block").get<int>());
        biome.subsurface_block =
            static_cast<voxel::core::BlockId>(entry.at("subsurface_block").get<int>());
        biome.filler_block = static_cast<voxel::core::BlockId>(entry.at("filler_block").get<int>());
        biome.height_base = entry.at("height_base").get<int>();
        biome.height_amplitude = entry.at("height_amplitude").get<float>();
        biome.tree_density = entry.at("tree_density").get<float>();
        biome.noise_frequency = entry.at("noise_frequency").get<float>();
        if (entry.contains("trees")) {
            for (const auto& treeId : entry.at("trees")) {
                biome.tree_ids.push_back(treeId.get<int>());
            }
        }
        const bool hasSnowLine = entry.contains("snow_line");
        const bool hasSnowBlock = entry.contains("snow_block");
        if (hasSnowLine || hasSnowBlock) {
            if (!hasSnowLine || !hasSnowBlock) {
                throw std::runtime_error("biome " + biome.name + ": snow_line and snow_block must both be set");
            }
            biome.has_snow_rule = true;
            biome.snow_line = entry.at("snow_line").get<int>();
            biome.snow_block =
                static_cast<voxel::core::BlockId>(entry.at("snow_block").get<int>());
        }
        data.biomes.push_back(biome);
    }

    for (const auto& entry : root.at("ores")) {
        OreDef ore;
        ore.block = static_cast<voxel::core::BlockId>(entry.at("block").get<int>());
        ore.name = entry.at("name").get<std::string>();
        ore.min_y = entry.at("min_y").get<int>();
        ore.max_y = entry.at("max_y").get<int>();
        ore.vein_frequency = entry.at("vein_frequency").get<float>();
        ore.vein_size = entry.at("vein_size").get<float>();
        data.ores.push_back(ore);
    }

    for (const auto& entry : root.at("trees")) {
        TreeDef tree;
        tree.id = entry.at("id").get<int>();
        tree.log_block = static_cast<voxel::core::BlockId>(entry.at("log_block").get<int>());
        if (entry.contains("leaves_block")) {
            tree.leaves_block = static_cast<voxel::core::BlockId>(entry.at("leaves_block").get<int>());
        } else {
            tree.leaves_block = 0;
        }
        tree.min_height = entry.at("min_height").get<int>();
        tree.max_height = entry.at("max_height").get<int>();
        tree.canopy_radius = entry.at("canopy_radius").get<int>();
        data.trees.push_back(tree);
    }

    return data;
}

std::string validateWorldGenSchemaV1(const WorldGenData& data, const voxel::core::BlocksFile& blocks) {
    if (data.schema != 1) {
        return "schema must be 1";
    }
    if (data.bedrock_layers < 1) {
        return "bedrock_layers must be >= 1";
    }
    if (data.region.size_x < 1 || data.region.size_y < 1 || data.region.size_z < 1) {
        return "region sizes must be positive";
    }
    if (data.height_noise.octaves < 1 || data.biome_noise.octaves < 1 || data.cave_noise.octaves < 1) {
        return "noise octaves must be >= 1";
    }
    if (data.biomes.size() < 4) {
        return "biomes array must contain at least 4 entries";
    }

    static const char* kRequiredBiomes[] = {"plains", "forest", "desert", "mountains"};
    for (const char* required : kRequiredBiomes) {
        bool found = false;
        for (const BiomeDef& biome : data.biomes) {
            if (biome.name == required) {
                found = true;
                break;
            }
        }
        if (!found) {
            return std::string("missing required biome: ") + required;
        }
    }

    std::unordered_set<int> biomeIds;
    for (size_t i = 0; i < data.biomes.size(); ++i) {
        const BiomeDef& biome = data.biomes[i];
        if (biome.id != static_cast<int>(i)) {
            return "biome id " + std::to_string(biome.id) + " out of order at index " + std::to_string(i);
        }
        if (!biomeIds.insert(biome.id).second) {
            return "duplicate biome id " + std::to_string(biome.id);
        }
        if (const std::string err = checkBlockId("biome surface_block", biome.surface_block, blocks);
            !err.empty()) {
            return err;
        }
        if (const std::string err =
                checkBlockId("biome subsurface_block", biome.subsurface_block, blocks);
            !err.empty()) {
            return err;
        }
        if (const std::string err = checkBlockId("biome filler_block", biome.filler_block, blocks);
            !err.empty()) {
            return err;
        }
        if (biome.tree_density < 0.0f || biome.tree_density > 1.0f) {
            return "biome tree_density must be in [0, 1]";
        }
        if (!biome.tree_ids.empty()) {
            std::unordered_set<int> seenTreeRefs;
            for (int treeId : biome.tree_ids) {
                if (!seenTreeRefs.insert(treeId).second) {
                    return "biome " + biome.name + ": duplicate tree id " + std::to_string(treeId);
                }
            }
        }
        if (biome.has_snow_rule) {
            if (const std::string err = checkBlockId("biome snow_block", biome.snow_block, blocks);
                !err.empty()) {
                return err;
            }
            if (biome.snow_line < 0 || biome.snow_line >= data.region.size_y) {
                return "biome " + biome.name + ": snow_line out of region bounds";
            }
        }
    }

    if (data.ores.empty()) {
        return "ores array must not be empty";
    }
    if (data.trees.empty()) {
        return "trees array must not be empty";
    }

    std::unordered_set<int> definedTreeIds;
    for (const TreeDef& tree : data.trees) {
        definedTreeIds.insert(tree.id);
    }

    for (const BiomeDef& biome : data.biomes) {
        for (int treeId : biome.tree_ids) {
            if (!definedTreeIds.contains(treeId)) {
                return "biome " + biome.name + ": unknown tree id " + std::to_string(treeId);
            }
        }
    }

    for (const OreDef& ore : data.ores) {
        if (const std::string err = checkBlockId("ore block", ore.block, blocks); !err.empty()) {
            return err;
        }
        if (ore.min_y > ore.max_y) {
            return "ore " + ore.name + ": min_y must be <= max_y";
        }
        if (ore.vein_frequency <= 0.0f) {
            return "ore " + ore.name + ": vein_frequency must be > 0";
        }
    }

    std::unordered_set<int> treeIds;
    for (const TreeDef& tree : data.trees) {
        if (!treeIds.insert(tree.id).second) {
            return "duplicate tree id " + std::to_string(tree.id);
        }
        if (tree.log_block == 0) {
            return "tree id " + std::to_string(tree.id) + ": log_block must not be air";
        }
        if (const std::string err = checkBlockId("tree log_block", tree.log_block, blocks); !err.empty()) {
            return err;
        }
        if (tree.leaves_block != 0) {
            if (const std::string err = checkBlockId("tree leaves_block", tree.leaves_block, blocks);
                !err.empty()) {
                return err;
            }
        }
        if (tree.min_height < 1 || tree.max_height < tree.min_height) {
            return "tree id " + std::to_string(tree.id) + ": invalid height range";
        }
        if (tree.canopy_radius < 0) {
            return "tree id " + std::to_string(tree.id) + ": canopy_radius must be >= 0";
        }
    }

    return {};
}

const BiomeDef& pickBiome(const WorldGenData& data, float biomeNoiseSample) {
    const float t = (biomeNoiseSample + 1.0f) * 0.5f;
    const size_t index =
        std::min(data.biomes.size() - 1, static_cast<size_t>(t * static_cast<float>(data.biomes.size())));
    return data.biomes[index];
}

const TreeDef* findTreeById(const WorldGenData& data, int treeId) {
    for (const TreeDef& tree : data.trees) {
        if (tree.id == treeId) {
            return &tree;
        }
    }
    return nullptr;
}

} // namespace voxel::gen
