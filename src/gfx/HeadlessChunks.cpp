#include "gfx/HeadlessChunks.h"

#include "core/Blocks.h"
#include "core/Config.h"
#include "core/Paths.h"
#include "core/World.h"
#include "gfx/Camera.h"
#include "gfx/ChunkRenderer.h"
#include "gfx/Frustum.h"
#include "gfx/TextureArray.h"
#include "gfx/TerrainCamera.h"
#include "gfx/TestCubeCamera.h"
#include "gfx/scene/TerrainScene.h"
#include "gfx/scene/TestCubeScene.h"
#include "world/WorldSession.h"

#include <spdlog/spdlog.h>

#include <stb_image_write.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace voxel::gfx {

namespace {

void logPixelStats(const std::vector<uint8_t>& rgb, uint32_t width, uint32_t height) {
    uint64_t sumR = 0;
    uint64_t sumG = 0;
    uint64_t sumB = 0;
    uint8_t minR = 255;
    uint8_t minG = 255;
    uint8_t minB = 255;
    uint8_t maxR = 0;
    uint8_t maxG = 0;
    uint8_t maxB = 0;
    size_t nonzero = 0;
    const size_t pixels = static_cast<size_t>(width) * height;
    for (size_t i = 0; i < pixels; ++i) {
        const uint8_t r = rgb[i * 3 + 0];
        const uint8_t g = rgb[i * 3 + 1];
        const uint8_t b = rgb[i * 3 + 2];
        sumR += r;
        sumG += g;
        sumB += b;
        minR = std::min(minR, r);
        minG = std::min(minG, g);
        minB = std::min(minB, b);
        maxR = std::max(maxR, r);
        maxG = std::max(maxG, g);
        maxB = std::max(maxB, b);
        if (r > 8 || g > 8 || b > 8) {
            ++nonzero;
        }
    }
    spdlog::info(
        "pixel stats: size={}x{} nonzero~bg={} R[{}..{}] G[{}..{}] B[{}..{}] mean=({:.1f},{:.1f},{:.1f})",
        width, height, nonzero, minR, maxR, minG, maxG, minB, maxB,
        static_cast<double>(sumR) / pixels, static_cast<double>(sumG) / pixels,
        static_cast<double>(sumB) / pixels);
}

} // namespace

void runHeadlessTestCubeScene(VkContext& ctx, int frames, uint32_t width, uint32_t height,
                              const std::string& outPath) {
    VkDevice device = ctx.device();
    VmaAllocator allocator = ctx.allocator();

    const bool terrainScene = voxel::core::Config::active().scene == "terrain";

    voxel::world::World world;
    const std::filesystem::path texDir = voxel::core::contentRoot() / "assets/textures";
    TextureArray textures(ctx, texDir.string());
    if (terrainScene) {
        scene::setupTerrainWorld(world);
    } else {
        const std::filesystem::path blocksPath = voxel::core::contentRoot() / "data/blocks.json";
        world.loadBlocks(blocksPath);
        scene::populateWorldFromTestCube(world);
        world.chunkManager.waitForPendingMeshes();
    }

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("test_cube vkCreateRenderPass failed");
    }

    VkImage colorImage = VK_NULL_HANDLE;
    VmaAllocation colorAlloc = VK_NULL_HANDLE;
    VkImageView colorView = VK_NULL_HANDLE;
    VkImage depthImage = VK_NULL_HANDLE;
    VmaAllocation depthAlloc = VK_NULL_HANDLE;
    VkImageView depthView = VK_NULL_HANDLE;

    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.extent = {width, height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        vmaCreateImage(allocator, &imageInfo, &allocInfo, &colorImage, &colorAlloc, nullptr);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = colorImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &viewInfo, nullptr, &colorView);
    }

    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_D32_SFLOAT;
        imageInfo.extent = {width, height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        vmaCreateImage(allocator, &imageInfo, &allocInfo, &depthImage, &depthAlloc, nullptr);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &viewInfo, nullptr, &depthView);
    }

    std::array<VkImageView, 2> fbAttachments = {colorView, depthView};
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    {
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
        fbInfo.pAttachments = fbAttachments.data();
        fbInfo.width = width;
        fbInfo.height = height;
        fbInfo.layers = 1;
        vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffer);
    }

    ChunkRenderer renderer(ctx, renderPass, textures, world.blocks);
    const auto blockQuery = [&world](int wx, int wy, int wz) { return world.getBlock(wx, wy, wz); };
    renderer.buildFromChunkManager(world.chunkManager, blockQuery);

    Camera camera = terrainScene ? makeTerrainGoldenCamera() : makeTestCubeCamera();

    Frustum frustum;
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    frustum.extractFromMatrix(camera.viewProjection(aspect));

    ChunkPushConstants push{};
    push.lightDir = glm::vec4(glm::normalize(glm::vec3(-0.35f, -1.0f, -0.25f)), 0.0f);
    push.fogColorFar = glm::vec4(0.55f, 0.65f, 0.82f, 160.0f);
    push.fogNear = 28.0f;
    push.fogFar = 160.0f;
    push.cameraCurvature = glm::vec4(glm::vec3(camera.position), 0.0f);
    push.enableCurvature = voxel::core::Config::active().planetCurvature ? 1u : 0u;
    if (push.enableCurvature != 0u) {
        push.cameraCurvature.w = 0.00008f;
    }

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

    VkFence fence = VK_NULL_HANDLE;
    {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(device, &fenceInfo, nullptr, &fence);
    }

    const VkClearValue clears[2] = {{{{0.12f, 0.14f, 0.20f, 1.0f}}}, {{1.0f, 0}}};

    for (int frame = 0; frame < frames; ++frame) {
        vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &fence);
        vkResetCommandBuffer(commandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        const VkImageLayout layoutBefore =
            (frame == 0) ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        VkImageMemoryBarrier toColor{};
        toColor.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toColor.srcAccessMask = (frame == 0) ? 0 : VK_ACCESS_TRANSFER_READ_BIT;
        toColor.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        toColor.oldLayout = layoutBefore;
        toColor.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        toColor.image = colorImage;
        toColor.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        toColor.subresourceRange.levelCount = 1;
        toColor.subresourceRange.layerCount = 1;
        const VkPipelineStageFlags srcStage =
            (frame == 0) ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_TRANSFER_BIT;
        vkCmdPipelineBarrier(commandBuffer, srcStage, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &toColor);

        if (frame == 0) {
            VkImageMemoryBarrier depthBarrier{};
            depthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            depthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthBarrier.image = depthImage;
            depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthBarrier.subresourceRange.levelCount = 1;
            depthBarrier.subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                 &depthBarrier);
        }

        VkRenderPassBeginInfo rpBegin{};
        rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBegin.renderPass = renderPass;
        rpBegin.framebuffer = framebuffer;
        rpBegin.renderArea.offset = {0, 0};
        rpBegin.renderArea.extent = {width, height};
        rpBegin.clearValueCount = 2;
        rpBegin.pClearValues = clears;

        vkCmdBeginRenderPass(commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.width = static_cast<float>(width);
        viewport.height = static_cast<float>(height);
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{{0, 0}, {width, height}};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        renderer.draw(commandBuffer, camera, aspect, frustum, push);
        vkCmdEndRenderPass(commandBuffer);
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &commandBuffer;
        vkQueueSubmit(ctx.graphicsQueue(), 1, &submit, fence);
    }

    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    spdlog::info(
        "{} culling: registered={} non_empty={} frustum_pass={} drawn={}",
        terrainScene ? "terrain" : "test_cube", renderer.chunkCount(), renderer.nonEmptyChunkCount(),
        renderer.frustumVisibleChunkCount(), renderer.drawnChunkCount());

    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingAlloc = VK_NULL_HANDLE;
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = imageSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                          VMA_ALLOCATION_CREATE_MAPPED_BIT;
        vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAlloc, nullptr);
    }

    vkResetCommandBuffer(commandBuffer, 0);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {width, height, 1};
    vkCmdCopyImageToBuffer(commandBuffer, colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer,
                           1, &region);
    vkEndCommandBuffer(commandBuffer);
    vkResetFences(device, 1, &fence);
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(ctx.graphicsQueue(), 1, &submit, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    void* mapped = nullptr;
    vmaMapMemory(allocator, stagingAlloc, &mapped);
    std::vector<uint8_t> rgba(static_cast<size_t>(imageSize));
    std::memcpy(rgba.data(), mapped, imageSize);
    vmaUnmapMemory(allocator, stagingAlloc);

    std::vector<uint8_t> rgb(static_cast<size_t>(width) * height * 3);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            const size_t src = (static_cast<size_t>(y) * width + x) * 4;
            const size_t dst = (static_cast<size_t>(y) * width + x) * 3;
            rgb[dst + 0] = rgba[src + 0];
            rgb[dst + 1] = rgba[src + 1];
            rgb[dst + 2] = rgba[src + 2];
        }
    }

    logPixelStats(rgb, width, height);

    if (!stbi_write_png(outPath.c_str(), static_cast<int>(width), static_cast<int>(height), 3, rgb.data(),
                        static_cast<int>(width * 3))) {
        throw std::runtime_error("stbi_write_png failed for " + outPath);
    }
    spdlog::info("Wrote {} ({}x{}, {} frame(s), scene={})", outPath, width, height, frames,
                 terrainScene ? "terrain" : "test_cube");

    voxel::world::WorldSession::clearActive();

    vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);
    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyFramebuffer(device, framebuffer, nullptr);
    vkDestroyImageView(device, depthView, nullptr);
    vmaDestroyImage(allocator, depthImage, depthAlloc);
    vkDestroyImageView(device, colorView, nullptr);
    vmaDestroyImage(allocator, colorImage, colorAlloc);
    vkDestroyRenderPass(device, renderPass, nullptr);
}

} // namespace voxel::gfx
