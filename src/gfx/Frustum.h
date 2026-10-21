#pragma once

#include <glm/glm.hpp>

#include <array>

namespace voxel::gfx {

struct Aabb {
    glm::vec3 min;
    glm::vec3 max;
};

class Frustum {
public:
    void extractFromMatrix(const glm::mat4& clip);

    [[nodiscard]] bool intersectsAabb(const Aabb& box) const;

private:
    std::array<glm::vec4, 6> planes_{};
};

} // namespace voxel::gfx
