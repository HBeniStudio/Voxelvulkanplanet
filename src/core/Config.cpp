#include "core/Config.h"

#include <cstring>
#include <stdexcept>

namespace voxel::core {

namespace {
Config g_activeConfig;
}

const Config& Config::active() {
    return g_activeConfig;
}

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
        } else if (std::strcmp(arg, "--width") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error("--width requires a value");
            }
            cfg.renderWidth = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if (std::strcmp(arg, "--height") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error("--height requires a value");
            }
            cfg.renderHeight = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if (std::strcmp(arg, "--scene") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error("--scene requires a value");
            }
            cfg.scene = argv[++i];
        } else if (std::strcmp(arg, "--planet-curvature") == 0) {
            cfg.planetCurvature = true;
        } else if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
            throw std::runtime_error(
                "usage: voxel [--headless] [--frames N] [--out path.png] [--scene name] "
                "[--log-level level] [--planet-curvature]");
        } else {
            throw std::runtime_error(std::string("unknown argument: ") + arg);
        }
    }
    g_activeConfig = cfg;
    return cfg;
}

} // namespace voxel::core
