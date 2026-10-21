#include "gfx/DesktopApp.h"

#include "core/Blocks.h"
#include "core/Config.h"
#include "core/Registry.h"
#include "core/World.h"
#include "gfx/Camera.h"
#include "gfx/ChunkRenderer.h"
#include "gfx/Frustum.h"
#include "gfx/Swapchain.h"
#include "gfx/VkContext.h"
#include "gfx/TestCubeCamera.h"
#include "gfx/TextureArray.h"
#include "gfx/scene/TerrainScene.h"
#include "gfx/scene/TestCubeScene.h"
#include "gfx/TerrainCamera.h"
#include "world/WorldSession.h"

#include <spdlog/spdlog.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <stdexcept>

namespace voxel::gfx {

namespace {

void updateFreeFlyCamera(Camera& camera, GLFWwindow* window, float dt) {
    const float moveSpeed = 12.0f;
    glm::dvec3 velocity{0.0};
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        velocity += glm::dvec3(camera.forward());
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        velocity -= glm::dvec3(camera.forward());
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        velocity -= glm::dvec3(camera.right());
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        velocity += glm::dvec3(camera.right());
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        velocity.y += 1.0;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        velocity.y -= 1.0;
    }
    if (glm::length(velocity) > 0.0) {
        camera.position += glm::normalize(velocity) * (moveSpeed * static_cast<double>(dt));
    }

    static bool firstMouse = true;
    static double lastX = 0.0;
    static double lastY = 0.0;
    double xpos = 0.0;
    double ypos = 0.0;
    glfwGetCursorPos(window, &xpos, &ypos);
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    const double dx = xpos - lastX;
    const double dy = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    const float sensitivity = 0.12f;
    camera.yaw += static_cast<float>(dx) * sensitivity;
    camera.pitch += static_cast<float>(dy) * sensitivity;
    camera.pitch = std::clamp(camera.pitch, -89.0f, 89.0f);
}

} // namespace

int runDesktopApp(const voxel::core::Config& cfg) {
    if (cfg.scene != "test_cube" && cfg.scene != "terrain") {
        spdlog::error("desktop mode supports --scene test_cube or --scene terrain");
        return 1;
    }
    const bool terrainScene = cfg.scene == "terrain";

    if (!glfwInit()) {
        spdlog::error("glfwInit failed");
        return 1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window =
        glfwCreateWindow(static_cast<int>(cfg.renderWidth), static_cast<int>(cfg.renderHeight),
                         "voxel", nullptr, nullptr);
    if (!window) {
        spdlog::error("glfwCreateWindow failed");
        glfwTerminate();
        return 1;
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    voxel::core::World world;

    VkContext ctx;
    Swapchain swapchain(ctx, window);
    const std::filesystem::path texDir = std::filesystem::path(VOXEL_SOURCE_DIR) / "assets/textures";
    TextureArray textures(ctx, texDir.string());
    if (terrainScene) {
        scene::setupTerrainWorld(world);
        voxel::world::WorldSession::setActive(&world);
    } else {
        world.loadBlocks(std::filesystem::path(VOXEL_SOURCE_DIR) / "data/blocks.json");
        scene::populateWorldFromTestCube(world);
        world.chunkManager.waitForPendingMeshes();
    }
    ChunkRenderer renderer(ctx, swapchain.renderPass(), textures, world.blocks);
    const auto blockQuery = [&world](int wx, int wy, int wz) { return world.getBlock(wx, wy, wz); };
    renderer.buildFromChunkManager(world.chunkManager, blockQuery);

    Camera camera = terrainScene ? makeTerrainGoldenCamera() : makeTestCubeCamera();
    size_t lastLoadedChunkCount = world.chunkManager.loadedChunkPositions().size();

    VkDevice device = ctx.device();
    VkCommandPool commandPool = VK_NULL_HANDLE;
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = ctx.graphicsQueueFamily();
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    }

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    }

    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;
    {
        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(device, &semInfo, nullptr, &imageAvailable);
        vkCreateSemaphore(device, &semInfo, nullptr, &renderFinished);
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence);
    }

    ChunkPushConstants push{};
    push.lightDir = glm::vec4(glm::normalize(glm::vec3(-0.35f, -1.0f, -0.25f)), 0.0f);
    push.fogColorFar = glm::vec4(0.55f, 0.65f, 0.82f, 160.0f);
    push.fogNear = 28.0f;
    push.fogFar = 160.0f;
    push.enableCurvature = cfg.planetCurvature ? 1u : 0u;
    if (push.enableCurvature != 0u) {
        push.cameraCurvature.w = 0.00008f;
    }

    auto lastTime = std::chrono::steady_clock::now();
    auto lastOverlay = lastTime;
    int frameCounter = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        swapchain.recreateIfNeeded();

        const auto now = std::chrono::steady_clock::now();
        const float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        updateFreeFlyCamera(camera, window, dt);
        if (terrainScene) {
            voxel::world::WorldSession::setStreamingPosition(camera.position);
            voxel::core::Registry::instance().runPhase(voxel::core::Phase::Update, world, dt);
            const size_t loadedCount = world.chunkManager.loadedChunkPositions().size();
            world.chunkManager.pumpCompletedMeshes();
            if (loadedCount != lastLoadedChunkCount) {
                world.chunkManager.waitForPendingMeshes();
                renderer.buildFromChunkManager(world.chunkManager, blockQuery);
                lastLoadedChunkCount = loadedCount;
            }
        }
        push.cameraCurvature = glm::vec4(glm::vec3(camera.position), push.cameraCurvature.w);

        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFence);

        uint32_t imageIndex = 0;
        const VkResult acquireResult = swapchain.acquireNextImage(imageAvailable, &imageIndex);
        if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
            continue;
        }

        vkResetCommandBuffer(commandBuffer, 0);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        const VkExtent2D extent = swapchain.extent();
        const float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
        Frustum frustum;
        frustum.extractFromMatrix(camera.viewProjection(aspect));

        const VkClearValue clears[2] = {{{{0.12f, 0.14f, 0.20f, 1.0f}}}, {{1.0f, 0}}};
        VkRenderPassBeginInfo rpBegin{};
        rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBegin.renderPass = swapchain.renderPass();
        rpBegin.framebuffer = swapchain.framebuffer(imageIndex);
        rpBegin.renderArea.offset = {0, 0};
        rpBegin.renderArea.extent = extent;
        rpBegin.clearValueCount = 2;
        rpBegin.pClearValues = clears;
        vkCmdBeginRenderPass(commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{{0, 0}, extent};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        renderer.draw(commandBuffer, camera, aspect, frustum, push);
        vkCmdEndRenderPass(commandBuffer);
        vkEndCommandBuffer(commandBuffer);

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &imageAvailable;
        submit.pWaitDstStageMask = &waitStage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &commandBuffer;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &renderFinished;
        vkQueueSubmit(ctx.graphicsQueue(), 1, &submit, inFlightFence);
        swapchain.present(imageIndex, renderFinished);

        ++frameCounter;
        const float overlayDt = std::chrono::duration<float>(now - lastOverlay).count();
        if (overlayDt >= 1.0f) {
            const float fps = static_cast<float>(frameCounter) / overlayDt;
            spdlog::info("overlay: fps={:.1f} chunks={}/{} (frustum_pass={})",
                         fps, renderer.drawnChunkCount(), renderer.nonEmptyChunkCount(),
                         renderer.frustumVisibleChunkCount());
            frameCounter = 0;
            lastOverlay = now;
        }

        voxel::core::Registry::instance().runPhase(voxel::core::Phase::Render, world, dt);
    }

    vkDeviceWaitIdle(device);
    vkDestroyFence(device, inFlightFence, nullptr);
    vkDestroySemaphore(device, renderFinished, nullptr);
    vkDestroySemaphore(device, imageAvailable, nullptr);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    vkDestroyCommandPool(device, commandPool, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
    ctx.shutdown();
    return 0;
}

} // namespace voxel::gfx
