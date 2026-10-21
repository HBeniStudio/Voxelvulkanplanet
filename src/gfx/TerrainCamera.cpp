#include "gfx/TerrainCamera.h"

namespace voxel::gfx {

Camera makeTerrainGoldenCamera() {
    Camera camera;
    camera.position = glm::dvec3(96.0, 78.0, 112.0);
    camera.yaw = -128.0f;
    camera.pitch = -22.0f;
    camera.fovYDegrees = 55.0f;
    camera.nearPlane = 0.1f;
    camera.farPlane = 512.0f;
    return camera;
}

} // namespace voxel::gfx
