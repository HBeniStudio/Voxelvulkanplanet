#pragma once

#include "gfx/VkContext.h"

#include <cstdint>
#include <string>

namespace voxel::gfx {

class Headless {
public:
    explicit Headless(VkContext& ctx);
    ~Headless();

    Headless(const Headless&) = delete;
    Headless& operator=(const Headless&) = delete;

    void run(int frames, uint32_t width, uint32_t height, const std::string& outPath);

private:
    VkContext& ctx_;
};

} // namespace voxel::gfx
