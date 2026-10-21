#include "world/WorldSession.h"

#include <cmath>

namespace voxel::world {

World* WorldSession::activeWorld_ = nullptr;
glm::dvec3 WorldSession::streamingPosition_{};
bool WorldSession::hasStreamingPosition_ = false;

void WorldSession::setActive(World* world) {
    activeWorld_ = world;
}

void WorldSession::clearActive() {
    activeWorld_ = nullptr;
    hasStreamingPosition_ = false;
}

World* WorldSession::active() {
    return activeWorld_;
}

void WorldSession::setStreamingPosition(const glm::dvec3& position) {
    streamingPosition_ = position;
    hasStreamingPosition_ = true;
}

glm::dvec3 WorldSession::streamingPosition() {
    return streamingPosition_;
}

bool WorldSession::hasStreamingPosition() {
    return hasStreamingPosition_;
}

ChunkPos chunkPosFromWorldPosition(const glm::dvec3& position) {
    const auto floorDiv = [](double value, int divisor) -> int32_t {
        return static_cast<int32_t>(std::floor(value / static_cast<double>(divisor)));
    };
    return {floorDiv(position.x, kChunkSize), floorDiv(position.y, kChunkSize),
            floorDiv(position.z, kChunkSize)};
}

} // namespace voxel::world
