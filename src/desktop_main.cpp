#include "core/Config.h"
#include "core/Log.h"
#include "core/Registry.h"
#include "core/World.h"
#include "gfx/DesktopApp.h"

#include <spdlog/spdlog.h>

int main(int argc, char** argv) {
    try {
        const voxel::core::Config cfg = voxel::core::Config::parse(argc, argv);
        voxel::core::Log::init(cfg.logLevel);

        voxel::core::World world;
        voxel::core::Registry::instance().runPhase(voxel::core::Phase::Startup, world, 0.0f);

        if (cfg.headless) {
            spdlog::error("voxel_desktop does not support --headless; use the voxel binary");
            return 1;
        }

        const int code = voxel::gfx::runDesktopApp(cfg);
        voxel::core::Registry::instance().runPhase(voxel::core::Phase::Shutdown, world, 0.0f);
        return code;
    } catch (const std::exception& ex) {
        spdlog::error("fatal: {}", ex.what());
        return 1;
    }
}
