#include "gfx/Frustum.h"

namespace voxel::gfx {

void Frustum::extractFromMatrix(const glm::mat4& m) {
    planes_[0] = glm::vec4(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]);
    planes_[1] = glm::vec4(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]);
    planes_[2] = glm::vec4(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]);
    planes_[3] = glm::vec4(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]);
    planes_[4] = glm::vec4(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2], m[3][3] + m[3][2]);
    planes_[5] = glm::vec4(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]);
    for (glm::vec4& p : planes_) {
        const float len = glm::length(glm::vec3(p));
        if (len > 0.0f) {
            p /= len;
        }
    }
}

bool Frustum::intersectsAabb(const Aabb& box) const {
    for (const glm::vec4& plane : planes_) {
        const glm::vec3 positive{
            plane.x >= 0.0f ? box.max.x : box.min.x,
            plane.y >= 0.0f ? box.max.y : box.min.y,
            plane.z >= 0.0f ? box.max.z : box.min.z,
        };
        if (glm::dot(glm::vec3(plane), positive) + plane.w < 0.0f) {
            return false;
        }
    }
    return true;
}

} // namespace voxel::gfx
