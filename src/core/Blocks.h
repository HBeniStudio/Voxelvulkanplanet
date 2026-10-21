#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace voxel::core {

using BlockId = uint16_t;

struct BlockTextureNames {
    std::string top;
    std::string side;
    std::string bottom;
};

struct BlockDef {
    BlockId id = 0;
    std::string name;
    bool solid = false;
    bool transparent = true;
    BlockTextureNames texture;
    float hardness = 0.0f;
    std::vector<std::string> drops;
};

struct BlocksFile {
    int schema = 0;
    std::vector<BlockDef> blocks;
};

[[nodiscard]] BlocksFile loadBlocksJson(const std::filesystem::path& path);
[[nodiscard]] std::string validateBlocksSchemaV1(const BlocksFile& file);

} // namespace voxel::core
