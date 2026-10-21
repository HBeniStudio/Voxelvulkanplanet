#pragma once

#include "gfx/VkContext.h"

#include <cstdint>
#include <string>

namespace voxel::gfx {

void runHeadlessTestCubeScene(VkContext& ctx, int frames, uint32_t width, uint32_t height,
                              const std::string& outPath);

} // namespace voxel::gfx
