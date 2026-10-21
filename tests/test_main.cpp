#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "core/Blocks.h"
#include "core/Log.h"
#include "core/Registry.h"
#include "core/World.h"
#include "gfx/Frustum.h"
#include "gfx/Headless.h"
#include "gfx/TestCubeCamera.h"
#include "gfx/VkContext.h"
#include "gfx/scene/TestCubeScene.h"

#include <glm/gtc/matrix_transform.hpp>

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

TEST_CASE("test_cube chunk frustums all intersect default camera") {
    const voxel::gfx::Camera camera = voxel::gfx::makeTestCubeCamera();
    voxel::gfx::Frustum frustum;
    frustum.extractFromMatrix(camera.viewProjection(1.0f));

    int inside = 0;
    for (int cz = -1; cz <= 1; ++cz) {
        for (int cy = -1; cy <= 1; ++cy) {
            for (int cx = -1; cx <= 1; ++cx) {
                const glm::vec3 origin(static_cast<float>(cx * voxel::gfx::scene::kChunkSize),
                                       static_cast<float>(cy * voxel::gfx::scene::kChunkSize),
                                       static_cast<float>(cz * voxel::gfx::scene::kChunkSize));
                const voxel::gfx::Aabb bounds{origin,
                                              origin + glm::vec3(voxel::gfx::scene::kChunkSize)};
                if (frustum.intersectsAabb(bounds)) {
                    ++inside;
                }
            }
        }
    }
    CHECK(inside >= 26);
    CHECK(inside <= 27);
}

TEST_CASE("frustum intersects axis-aligned boxes") {
    voxel::gfx::Frustum frustum;
    const glm::mat4 clip = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 100.0f) *
                           glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    frustum.extractFromMatrix(clip);

    const voxel::gfx::Aabb inside{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
    CHECK(frustum.intersectsAabb(inside));

    const voxel::gfx::Aabb behindCamera{{0.0f, 0.0f, 20.0f}, {1.0f, 1.0f, 21.0f}};
    CHECK_FALSE(frustum.intersectsAabb(behindCamera));

    const voxel::gfx::Aabb farOutside{{200.0f, 200.0f, 200.0f}, {201.0f, 201.0f, 201.0f}};
    CHECK_FALSE(frustum.intersectsAabb(farOutside));
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
