#pragma once

#include <volk.h>

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace voxel::gfx {

inline VkShaderModule loadShaderModule(VkDevice device, const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error(std::string("failed to open shader: ") + path);
    }
    const auto size = file.tellg();
    file.seekg(0);
    std::vector<char> buffer(static_cast<size_t>(size));
    file.read(buffer.data(), size);

    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = buffer.size();
    info.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    VkShaderModule module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &info, nullptr, &module) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateShaderModule failed");
    }
    return module;
}

} // namespace voxel::gfx
