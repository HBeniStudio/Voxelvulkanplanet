#include "gfx/Camera.h"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace voxel::gfx {

glm::vec3 Camera::forward() const {
    const float yawRad = glm::radians(yaw);
    const float pitchRad = glm::radians(pitch);
    glm::vec3 dir;
    dir.x = std::cos(yawRad) * std::cos(pitchRad);
    dir.y = std::sin(pitchRad);
    dir.z = std::sin(yawRad) * std::cos(pitchRad);
    return glm::normalize(dir);
}

glm::vec3 Camera::right() const {
    return glm::normalize(glm::cross(forward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 Camera::up() const {
    return glm::normalize(glm::cross(right(), forward()));
}

glm::mat4 Camera::viewMatrix() const {
    const glm::vec3 eye(position);
    return glm::lookAt(eye, eye + forward(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::projectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(fovYDegrees), aspect, nearPlane, farPlane);
}

glm::mat4 Camera::viewProjection(float aspect) const {
    return projectionMatrix(aspect) * viewMatrix();
}

} // namespace voxel::gfx
