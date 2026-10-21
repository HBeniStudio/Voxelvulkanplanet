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
    std::string scene = "triangle";
    bool planetCurvature = false;
    uint64_t worldSeed = 42;
    int streamHorizontalDistance = 8;
    int streamVerticalDistance = 4;

    static Config parse(int argc, char** argv);
    static const Config& active();
};

} // namespace voxel::core
