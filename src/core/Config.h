#pragma once

#include <cstdint>
#include <string>

namespace voxel::core {

struct Config {
    bool headless = false;
    int frames = 1;
    std::string outPath = "frame.png";
    std::string logLevel = "info";
    uint32_t renderWidth = 256;
    uint32_t renderHeight = 256;

    static Config parse(int argc, char** argv);
};

} // namespace voxel::core
