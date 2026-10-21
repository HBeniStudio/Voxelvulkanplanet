#pragma once

#include "gfx/VkContext.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace voxel::gfx {

class TextureArray {
public:
    TextureArray(VkContext& ctx, const std::string& assetsTexturesDir);
    ~TextureArray();

    TextureArray(const TextureArray&) = delete;
    TextureArray& operator=(const TextureArray&) = delete;

    [[nodiscard]] VkImageView view() const { return view_; }
    [[nodiscard]] VkSampler sampler() const { return sampler_; }
    [[nodiscard]] uint32_t layerForName(const std::string& name) const;

private:
    VkContext& ctx_;
    VkImage image_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
    VkImageView view_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
    std::unordered_map<std::string, uint32_t> layerByName_;
};

} // namespace voxel::gfx
