#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "core/Blocks.h"
#include "core/Log.h"
#include "core/Registry.h"
#include "core/World.h"
#include "gfx/Headless.h"
#include "gfx/VkContext.h"

#include <algorithm>
#include <filesystem>

namespace {

class RegistryOrderA final : public voxel::core::ISystem {
public:
    void update(voxel::core::World&, float) override {}
};

class RegistryOrderB final : public voxel::core::ISystem {
public:
    void update(voxel::core::World&, float) override {}
};

REGISTER_SYSTEM(RegistryOrderA, voxel::core::Phase::Update);
REGISTER_SYSTEM(RegistryOrderB, voxel::core::Phase::Update);

} // namespace

TEST_CASE("registry preserves registration order within a phase") {
    const auto names = voxel::core::Registry::instance().systemNamesForPhase(voxel::core::Phase::Update);
    const auto posA = std::find(names.begin(), names.end(), "RegistryOrderA");
    const auto posB = std::find(names.begin(), names.end(), "RegistryOrderB");
    REQUIRE(posA != names.end());
    REQUIRE(posB != names.end());
    CHECK(posA < posB);
}

TEST_CASE("blocks.json schema v1 loads and validates frozen ids 0..12") {
    const std::filesystem::path path = std::filesystem::path(VOXEL_SOURCE_DIR) / "data/blocks.json";
    voxel::core::World world;
    world.loadBlocks(path);
    CHECK(world.blocks.schema == 1);
    CHECK(world.blocks.blocks.size() == 13);
    for (voxel::core::BlockId id = 0; id <= 12; ++id) {
        CHECK(world.blocks.blocks[id].id == id);
    }
}

TEST_CASE("headless triangle renders to a png") {
    voxel::core::Log::init("error");
    voxel::gfx::VkContext vk;
    REQUIRE(vk.initialize(true));

    const std::filesystem::path out =
        std::filesystem::path(VOXEL_SOURCE_DIR) / "build/tests-out/headless_triangle.png";
    std::filesystem::create_directories(out.parent_path());

    voxel::gfx::Headless headless(vk);
    headless.run(1, 256, 256, out.string());

    REQUIRE(std::filesystem::exists(out));
    CHECK(std::filesystem::file_size(out) > 1000);
    vk.shutdown();
}
