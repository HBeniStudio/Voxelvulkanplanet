#pragma once

#include <filesystem>

namespace voxel::core {

// Root containing data/blocks.json (release zip layout or dev tree).
std::filesystem::path contentRoot();

// Directory containing compiled *.spv shaders.
std::filesystem::path shaderDir();

} // namespace voxel::core
