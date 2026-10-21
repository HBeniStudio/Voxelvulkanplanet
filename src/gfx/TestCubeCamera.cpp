#include "gfx/TestCubeCamera.h"

namespace voxel::gfx {

Camera makeTestCubeCamera() {
    Camera camera;
    camera.position = glm::dvec3(52.0, 44.0, 52.0);
    camera.yaw = -135.0f;
    camera.pitch = -26.0f;
    camera.fovYDegrees = 50.0f;
    camera.nearPlane = 0.1f;
    camera.farPlane = 256.0f;
    return camera;
}

} // namespace voxel::gfx
