#pragma once

#include <glm/glm.hpp>

namespace voxel::gfx {

class Camera {
public:
    glm::dvec3 position{0.0, 2.0, 6.0};
    float yaw = -90.0f;
    float pitch = -20.0f;
    float fovYDegrees = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 512.0f;

    [[nodiscard]] glm::vec3 forward() const;
    [[nodiscard]] glm::vec3 right() const;
    [[nodiscard]] glm::vec3 up() const;

    [[nodiscard]] glm::mat4 viewMatrix() const;
    [[nodiscard]] glm::mat4 projectionMatrix(float aspect) const;
    [[nodiscard]] glm::mat4 viewProjection(float aspect) const;
};

} // namespace voxel::gfx
