#include "core/Config.h"
#include "core/Log.h"
#include "core/Registry.h"
#include "core/World.h"
#include "gfx/Headless.h"
#include "gfx/VkContext.h"

#include <spdlog/spdlog.h>

int main(int argc, char** argv) {
    try {
        const voxel::core::Config cfg = voxel::core::Config::parse(argc, argv);
        voxel::core::Log::init(cfg.logLevel);

        voxel::core::World world;
        voxel::core::Registry::instance().runPhase(voxel::core::Phase::Startup, world, 0.0f);

        if (!cfg.headless) {
            spdlog::error("P1 supports headless mode only. Pass --headless --frames N --out frame.png");
            return 1;
        }

        voxel::gfx::VkContext vk;
        if (!vk.initialize(true)) {
            spdlog::error("Vulkan initialization failed");
            return 1;
        }

        voxel::gfx::Headless headless(vk);
        headless.run(cfg.frames, cfg.renderWidth, cfg.renderHeight, cfg.outPath);

        voxel::core::Registry::instance().runPhase(voxel::core::Phase::Update, world, 0.0f);
        voxel::core::Registry::instance().runPhase(voxel::core::Phase::Shutdown, world, 0.0f);

        vk.shutdown();
        spdlog::info("done");
        return 0;
    } catch (const std::exception& ex) {
        spdlog::error("fatal: {}", ex.what());
        return 1;
    }
}
