#include "core/Registry.h"

#include <spdlog/spdlog.h>

namespace {

class ChunkRendererStartupSystem final : public voxel::core::ISystem {
public:
    void update(voxel::core::World&, float) override {
        spdlog::debug("ChunkRendererStartupSystem registered");
    }
};

REGISTER_SYSTEM(ChunkRendererStartupSystem, voxel::core::Phase::Startup);

} // namespace
