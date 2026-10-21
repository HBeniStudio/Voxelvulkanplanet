#pragma once

#include "world/Chunk.h"
#include "world/World.h"

#include <glm/glm.hpp>

namespace voxel::world {

class WorldSession {
public:
    static void setActive(World* world);
    static void clearActive();
    [[nodiscard]] static World* active();

    static void setStreamingPosition(const glm::dvec3& position);
    [[nodiscard]] static glm::dvec3 streamingPosition();
    [[nodiscard]] static bool hasStreamingPosition();

private:
    static World* activeWorld_;
    static glm::dvec3 streamingPosition_;
    static bool hasStreamingPosition_;
};

[[nodiscard]] ChunkPos chunkPosFromWorldPosition(const glm::dvec3& position);

} // namespace voxel::world
