#include "gfx/ChunkRenderer.h"

#include "gfx/GfxShaderUtil.h"

#include <spdlog/spdlog.h>

#include <array>
#include <cstring>
#include <stdexcept>

namespace voxel::gfx {

namespace {

constexpr std::array<glm::ivec3, 6> kFaceNormals = {{
    {1, 0, 0},
    {-1, 0, 0},
    {0, 1, 0},
    {0, -1, 0},
    {0, 0, 1},
    {0, 0, -1},
}};

constexpr std::array<glm::ivec3, 6> kFaceTangentU = {{
    {0, 0, -1},
    {0, 0, 1},
    {-1, 0, 0},
    {-1, 0, 0},
    {1, 0, 0},
    {-1, 0, 0},
}};

constexpr std::array<glm::ivec3, 6> kFaceTangentV = {{
    {0, 1, 0},
    {0, 1, 0},
    {0, 0, 1},
    {0, 0, -1},
    {0, 1, 0},
    {0, 1, 0},
}};

enum class FaceName { Top, Bottom, Side };

FaceName faceNameForNormal(const glm::ivec3& n) {
    if (n.y > 0) {
        return FaceName::Top;
    }
    if (n.y < 0) {
        return FaceName::Bottom;
    }
    return FaceName::Side;
}

std::string textureNameForBlock(const voxel::core::BlockDef& def, FaceName face) {
    switch (face) {
    case FaceName::Top:
        return def.texture.top.empty() ? def.texture.side : def.texture.top;
    case FaceName::Bottom:
        return def.texture.bottom.empty() ? def.texture.side : def.texture.bottom;
    case FaceName::Side:
        return def.texture.side;
    }
    return def.texture.side;
}

void pushFace(std::vector<ChunkVertex>& out, const glm::vec3& origin, const glm::ivec3& normal,
              const glm::ivec3& tangentU, const glm::ivec3& tangentV, float texLayer) {
    const glm::vec3 n = glm::vec3(normal);
    const glm::vec3 u = glm::vec3(tangentU);
    const glm::vec3 v = glm::vec3(tangentV);
    const glm::vec3 base = origin;
    const glm::vec3 p0 = base;
    const glm::vec3 p1 = base + u;
    const glm::vec3 p2 = base + u + v;
    const glm::vec3 p3 = base + v;

    const ChunkVertex verts[6] = {
        {{p0.x, p0.y, p0.z}, {n.x, n.y, n.z}, {0.0f, 0.0f}, texLayer},
        {{p1.x, p1.y, p1.z}, {n.x, n.y, n.z}, {1.0f, 0.0f}, texLayer},
        {{p2.x, p2.y, p2.z}, {n.x, n.y, n.z}, {1.0f, 1.0f}, texLayer},
        {{p0.x, p0.y, p0.z}, {n.x, n.y, n.z}, {0.0f, 0.0f}, texLayer},
        {{p2.x, p2.y, p2.z}, {n.x, n.y, n.z}, {1.0f, 1.0f}, texLayer},
        {{p3.x, p3.y, p3.z}, {n.x, n.y, n.z}, {0.0f, 1.0f}, texLayer},
    };
    out.insert(out.end(), std::begin(verts), std::end(verts));
}

} // namespace

ChunkRenderer::ChunkRenderer(VkContext& ctx, VkRenderPass renderPass, TextureArray& textures,
                             const voxel::core::BlocksFile& blocks)
    : ctx_(ctx), textures_(textures), blocks_(blocks) {
    createPipeline(renderPass);
}

ChunkRenderer::~ChunkRenderer() {
    VkDevice device = ctx_.device();
    VmaAllocator allocator = ctx_.allocator();
    for (ChunkGpuMesh& mesh : meshes_) {
        if (mesh.vertexBuffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, mesh.vertexBuffer, mesh.allocation);
        }
    }
    if (descriptorPool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool_, nullptr);
    }
    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, pipeline_, nullptr);
    }
    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
    }
    if (descriptorLayout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorLayout_, nullptr);
    }
}

void ChunkRenderer::createPipeline(VkRenderPass renderPass) {
    VkDevice device = ctx_.device();

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorLayout_) != VK_SUCCESS) {
        throw std::runtime_error("ChunkRenderer descriptor set layout failed");
    }

    VkPushConstantRange push{};
    push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    push.offset = 0;
    push.size = sizeof(ChunkPushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorLayout_;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &push;
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        throw std::runtime_error("ChunkRenderer pipeline layout failed");
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
        throw std::runtime_error("ChunkRenderer descriptor pool failed");
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorLayout_;
    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet_) != VK_SUCCESS) {
        throw std::runtime_error("ChunkRenderer descriptor set alloc failed");
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textures_.view();
    imageInfo.sampler = textures_.sampler();
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet_;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    const std::string vertSpv = std::string(VOXEL_SHADER_DIR) + "/chunk.vert.spv";
    const std::string fragSpv = std::string(VOXEL_SHADER_DIR) + "/chunk.frag.spv";
    VkShaderModule vertModule = loadShaderModule(device, vertSpv.c_str());
    VkShaderModule fragModule = loadShaderModule(device, fragSpv.c_str());

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";

    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(ChunkVertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[4]{};
    attrs[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ChunkVertex, pos)};
    attrs[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ChunkVertex, normal)};
    attrs[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ChunkVertex, uv)};
    attrs[3] = {3, 0, VK_FORMAT_R32_SFLOAT, offsetof(ChunkVertex, texLayer)};

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = 4;
    vertexInput.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_BACK_BIT;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAttachment;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &raster;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &blend;
    pipelineInfo.pDynamicState = &dynamic;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_) !=
        VK_SUCCESS) {
        throw std::runtime_error("ChunkRenderer pipeline failed");
    }

    vkDestroyShaderModule(device, fragModule, nullptr);
    vkDestroyShaderModule(device, vertModule, nullptr);
}

void ChunkRenderer::uploadMesh(ChunkGpuMesh& mesh, const std::vector<ChunkVertex>& vertices) {
    mesh.vertexCount = static_cast<uint32_t>(vertices.size());
    if (vertices.empty()) {
        return;
    }

    VkDevice device = ctx_.device();
    VmaAllocator allocator = ctx_.allocator();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(ChunkVertex) * vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT;

    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &mesh.vertexBuffer, &mesh.allocation,
                        nullptr) != VK_SUCCESS) {
        throw std::runtime_error("ChunkRenderer vmaCreateBuffer failed");
    }

    void* mapped = nullptr;
    vmaMapMemory(allocator, mesh.allocation, &mapped);
    std::memcpy(mapped, vertices.data(), bufferInfo.size);
    vmaUnmapMemory(allocator, mesh.allocation);
}

void ChunkRenderer::buildFromScene(const scene::TestCubeScene& scene) {
    meshes_.clear();
    const auto positions = scene.chunkPositions();
    meshes_.reserve(positions.size());

    for (const scene::ChunkPosKey& pos : positions) {
        ChunkGpuMesh mesh{};
        mesh.pos = pos;
        const glm::vec3 chunkOrigin(static_cast<float>(pos.x * scene::kChunkSize),
                                  static_cast<float>(pos.y * scene::kChunkSize),
                                  static_cast<float>(pos.z * scene::kChunkSize));
        mesh.bounds = {chunkOrigin, chunkOrigin + glm::vec3(scene::kChunkSize)};

        std::vector<ChunkVertex> vertices;
        const auto& voxels = scene.chunks.at(pos);

        for (int z = 0; z < scene::kChunkSize; ++z) {
            for (int y = 0; y < scene::kChunkSize; ++y) {
                for (int x = 0; x < scene::kChunkSize; ++x) {
                    const size_t idx =
                        static_cast<size_t>(x + y * scene::kChunkSize + z * scene::kChunkSize * scene::kChunkSize);
                    const voxel::core::BlockId id = voxels[idx];
                    if (id == 0 || id >= blocks_.blocks.size()) {
                        continue;
                    }
                    const auto& def = blocks_.blocks[id];
                    if (!def.solid) {
                        continue;
                    }

                    const glm::vec3 blockOrigin = chunkOrigin + glm::vec3(static_cast<float>(x),
                                                                           static_cast<float>(y),
                                                                           static_cast<float>(z));

                    for (size_t face = 0; face < kFaceNormals.size(); ++face) {
                        const glm::ivec3 n = kFaceNormals[face];
                        const int wx = pos.x * scene::kChunkSize + x + n.x;
                        const int wy = pos.y * scene::kChunkSize + y + n.y;
                        const int wz = pos.z * scene::kChunkSize + z + n.z;
                        const voxel::core::BlockId neighbor = scene.blockAtWorld(wx, wy, wz);
                        bool neighborSolid = false;
                        if (neighbor != 0 && neighbor < blocks_.blocks.size()) {
                            neighborSolid = blocks_.blocks[neighbor].solid &&
                                            !blocks_.blocks[neighbor].transparent;
                        }
                        if (neighborSolid) {
                            continue;
                        }

                        const FaceName faceName = faceNameForNormal(n);
                        const std::string texName = textureNameForBlock(def, faceName);
                        const float layer = static_cast<float>(textures_.layerForName(texName));

                        glm::vec3 origin = blockOrigin;
                        if (n.x > 0) {
                            origin.x += 1.0f;
                        }
                        if (n.y > 0) {
                            origin.y += 1.0f;
                        }
                        if (n.z > 0) {
                            origin.z += 1.0f;
                        }

                        pushFace(vertices, origin, n, kFaceTangentU[face], kFaceTangentV[face], layer);
                    }
                }
            }
        }

        uploadMesh(mesh, vertices);
        meshes_.push_back(std::move(mesh));
    }

    spdlog::info("ChunkRenderer: built {} chunk meshes", meshes_.size());
}

void ChunkRenderer::draw(VkCommandBuffer cmd, const Camera& camera, float aspect, const Frustum& frustum,
                         const ChunkPushConstants& extra) {
    lastNonEmpty_ = 0;
    lastFrustumVisible_ = 0;
    lastDrawn_ = 0;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, 1, &descriptorSet_,
                            0, nullptr);

    const glm::mat4 vp = camera.viewProjection(aspect);
    for (const ChunkGpuMesh& mesh : meshes_) {
        if (mesh.vertexCount == 0) {
            continue;
        }
        ++lastNonEmpty_;
        if (!frustum.intersectsAabb(mesh.bounds)) {
            continue;
        }
        ++lastFrustumVisible_;
        ++lastDrawn_;

        ChunkPushConstants pc = extra;
        pc.mvp = vp;
        vkCmdPushConstants(cmd, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(ChunkPushConstants), &pc);

        const VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer, &offset);
        vkCmdDraw(cmd, mesh.vertexCount, 1, 0, 0);
    }
}

} // namespace voxel::gfx
