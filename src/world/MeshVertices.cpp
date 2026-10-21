#include "world/MeshVertices.h"

#include "world/Mesher.h"

#include <array>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace voxel::world {

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

void pushFace(std::vector<MeshVertex>& out, const glm::vec3& origin, const glm::vec3& normal,
              const glm::vec3& tangentU, const glm::vec3& tangentV, float texLayer) {
    const glm::vec3 p0 = origin;
    const glm::vec3 p1 = origin + tangentU;
    const glm::vec3 p2 = origin + tangentU + tangentV;
    const glm::vec3 p3 = origin + tangentV;

    const MeshVertex verts[6] = {
        {{p0.x, p0.y, p0.z}, {normal.x, normal.y, normal.z}, {0.0f, 0.0f}, texLayer},
        {{p1.x, p1.y, p1.z}, {normal.x, normal.y, normal.z}, {1.0f, 0.0f}, texLayer},
        {{p2.x, p2.y, p2.z}, {normal.x, normal.y, normal.z}, {1.0f, 1.0f}, texLayer},
        {{p0.x, p0.y, p0.z}, {normal.x, normal.y, normal.z}, {0.0f, 0.0f}, texLayer},
        {{p2.x, p2.y, p2.z}, {normal.x, normal.y, normal.z}, {1.0f, 1.0f}, texLayer},
        {{p3.x, p3.y, p3.z}, {normal.x, normal.y, normal.z}, {0.0f, 1.0f}, texLayer},
    };
    out.insert(out.end(), std::begin(verts), std::end(verts));
}

int faceIndexForQuad(const MeshQuad& quad) {
    return quad.axis * 2 + (quad.positive ? 0 : 1);
}

} // namespace

std::vector<MeshVertex> buildVerticesFromQuads(const ChunkPos& pos, const std::vector<MeshQuad>& quads,
                                               const voxel::core::BlocksFile& blocks,
                                               TextureLayerFn layerForTexture) {
    std::vector<MeshVertex> vertices;
    const glm::vec3 chunkOrigin(static_cast<float>(pos.x * kChunkSize),
                                static_cast<float>(pos.y * kChunkSize),
                                static_cast<float>(pos.z * kChunkSize));

    for (const MeshQuad& quad : quads) {
        if (quad.blockId >= blocks.blocks.size()) {
            continue;
        }
        const voxel::core::BlockDef& def = blocks.blocks[quad.blockId];
        const int faceIdx = faceIndexForQuad(quad);
        const glm::ivec3 normal = kFaceNormals[static_cast<size_t>(faceIdx)];
        const glm::ivec3 tanU = kFaceTangentU[static_cast<size_t>(faceIdx)];
        const glm::ivec3 tanV = kFaceTangentV[static_cast<size_t>(faceIdx)];

        const int along = quad.positive ? quad.slice : (quad.slice - 1);
        glm::vec3 origin = chunkOrigin;
        if (quad.axis == 0) {
            origin += glm::vec3(static_cast<float>(along), static_cast<float>(quad.u),
                                static_cast<float>(quad.v));
        } else if (quad.axis == 1) {
            origin += glm::vec3(static_cast<float>(quad.u), static_cast<float>(along),
                                static_cast<float>(quad.v));
        } else {
            origin += glm::vec3(static_cast<float>(quad.u), static_cast<float>(quad.v),
                                static_cast<float>(along));
        }

        const FaceName faceName = faceNameForNormal(normal);
        const std::string texName = textureNameForBlock(def, faceName);
        const float layer = layerForTexture(texName);

        const glm::vec3 u = glm::vec3(tanU) * static_cast<float>(quad.width);
        const glm::vec3 v = glm::vec3(tanV) * static_cast<float>(quad.height);
        pushFace(vertices, origin, glm::vec3(normal), u, v, layer);
    }
    return vertices;
}

std::vector<MeshVertex> buildChunkMeshVertices(const ChunkPos& pos, const ChunkStorage& storage,
                                               BlockQueryFn blockAtWorld,
                                               const voxel::core::BlocksFile& blocks,
                                               TextureLayerFn layerForTexture) {
    const MesherResult meshed = meshChunkGreedy(pos, storage, blockAtWorld, blocks);
    return buildVerticesFromQuads(pos, meshed.quads, blocks, layerForTexture);
}

} // namespace voxel::world
