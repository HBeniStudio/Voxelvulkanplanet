#pragma once

namespace voxel::world {

struct MeshVertex {
    float pos[3];
    float normal[3];
    float uv[2];
    float texLayer;
};

} // namespace voxel::world
