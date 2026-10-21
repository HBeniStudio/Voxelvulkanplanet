#include "core/Config.h"

#include <cstring>
#include <stdexcept>

namespace voxel::core {

Config Config::parse(int argc, char** argv) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (std::strcmp(arg, "--headless") == 0) {
            cfg.headless = true;
        } else if (std::strcmp(arg, "--frames") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error("--frames requires a value");
            }
            cfg.frames = std::stoi(argv[++i]);
            if (cfg.frames < 1) {
                throw std::runtime_error("--frames must be >= 1");
            }
        } else if (std::strcmp(arg, "--out") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error("--out requires a path");
            }
            cfg.outPath = argv[++i];
        } else if (std::strcmp(arg, "--log-level") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error("--log-level requires a value");
            }
            cfg.logLevel = argv[++i];
        } else if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
            throw std::runtime_error(
                "usage: voxel [--headless] [--frames N] [--out path.png] [--log-level level]");
        } else {
            throw std::runtime_error(std::string("unknown argument: ") + arg);
        }
    }
    return cfg;
}

} // namespace voxel::core
