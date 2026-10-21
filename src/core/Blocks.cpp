#include "core/Blocks.h"

#include <array>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <unordered_set>

namespace voxel::core {

namespace {

using json = nlohmann::json;

const std::array<const char*, 13> kFrozenNames = {
    "air",         "stone",       "dirt",        "grass",      "sand",        "water",
    "log",         "leaves",      "coal_ore",    "iron_ore",   "bedrock",     "planks",
    "crafting_table",
};

BlockDef parseBlock(const json& node) {
    BlockDef block;
    block.id = static_cast<BlockId>(node.at("id").get<int>());
    block.name = node.at("name").get<std::string>();
    block.solid = node.at("solid").get<bool>();
    block.transparent = node.at("transparent").get<bool>();
    const auto& tex = node.at("texture");
    block.texture.top = tex.at("top").get<std::string>();
    block.texture.side = tex.at("side").get<std::string>();
    block.texture.bottom = tex.at("bottom").get<std::string>();
    block.hardness = node.at("hardness").get<float>();
    if (node.contains("drops")) {
        for (const auto& drop : node.at("drops")) {
            if (drop.is_string()) {
                block.drops.push_back(drop.get<std::string>());
            } else if (drop.is_number_integer()) {
                block.drops.push_back(std::to_string(drop.get<int>()));
            }
        }
    }
    return block;
}

} // namespace

BlocksFile loadBlocksJson(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("failed to open blocks file: " + path.string());
    }
    json root = json::parse(in);
    BlocksFile file;
    file.schema = root.at("schema").get<int>();
    for (const auto& entry : root.at("blocks")) {
        file.blocks.push_back(parseBlock(entry));
    }
    return file;
}

std::string validateBlocksSchemaV1(const BlocksFile& file) {
    if (file.schema != 1) {
        return "schema must be 1";
    }
    if (file.blocks.size() != 13) {
        return "expected exactly 13 blocks (ids 0..12)";
    }
    std::unordered_set<BlockId> seen;
    for (size_t i = 0; i < file.blocks.size(); ++i) {
        const BlockDef& block = file.blocks[i];
        if (block.id != static_cast<BlockId>(i)) {
            return "block id " + std::to_string(block.id) + " out of order at index " + std::to_string(i);
        }
        if (!seen.insert(block.id).second) {
            return "duplicate block id " + std::to_string(block.id);
        }
        if (block.name != kFrozenNames[i]) {
            return "block id " + std::to_string(block.id) + " name mismatch: expected " + kFrozenNames[i] +
                   ", got " + block.name;
        }
        if (block.id == 0) {
            if (block.solid || !block.transparent) {
                return "air must be non-solid and transparent";
            }
        }
    }
    return {};
}

} // namespace voxel::core
