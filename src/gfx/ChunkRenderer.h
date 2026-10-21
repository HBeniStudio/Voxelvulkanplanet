#pragma once

#include "core/Blocks.h"
#include "gfx/Camera.h"
#include "gfx/Frustum.h"
#include "gfx/TextureArray.h"
#include "gfx/VkContext.h"
#include "gfx/scene/TestCubeScene.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace voxel::gfx {

struct ChunkVertex {
    float pos[3];
    float normal[3];
    float uv[2];
    float texLayer;
};

struct ChunkGpuMesh {
    scene::ChunkPosKey pos;
    Aabb bounds;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    uint32_t vertexCount = 0;
};

struct ChunkPushConstants {
    alignas(16) glm::mat4 mvp;
    alignas(16) glm::vec4 lightDir;
    alignas(16) glm::vec4 fogColorFar;
    alignas(16) glm::vec4 cameraCurvature;
    float fogNear = 32.0f;
    float fogFar = 192.0f;
    uint32_t enableCurvature = 0;
};

class ChunkRenderer {
public:
    ChunkRenderer(VkContext& ctx, VkRenderPass renderPass, TextureArray& textures,
                  const voxel::core::BlocksFile& blocks);
    ~ChunkRenderer();

    ChunkRenderer(const ChunkRenderer&) = delete;
    ChunkRenderer& operator=(const ChunkRenderer&) = delete;

    void buildFromScene(const scene::TestCubeScene& scene);
    void draw(VkCommandBuffer cmd, const Camera& camera, float aspect, const Frustum& frustum,
              const ChunkPushConstants& extra);

    [[nodiscard]] VkPipeline pipeline() const { return pipeline_; }
    [[nodiscard]] VkPipelineLayout layout() const { return pipelineLayout_; }
    [[nodiscard]] size_t chunkCount() const { return meshes_.size(); }
    [[nodiscard]] size_t visibleChunkCount() const { return lastVisible_; }

private:
    VkContext& ctx_;
    TextureArray& textures_;
    const voxel::core::BlocksFile& blocks_;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
    std::vector<ChunkGpuMesh> meshes_;
    size_t lastVisible_ = 0;

    void createPipeline(VkRenderPass renderPass);
    void uploadMesh(ChunkGpuMesh& mesh, const std::vector<ChunkVertex>& vertices);
};

} // namespace voxel::gfx
