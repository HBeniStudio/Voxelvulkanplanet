cmake_minimum_required(VERSION 3.25)
project(voxel VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(volk CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(VulkanHeaders CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)
find_package(Stb REQUIRED)

find_program(GLSLC_EXECUTABLE glslc REQUIRED)

set(SHADER_SOURCES
    shaders/tri.vert
    shaders/tri.frag
    shaders/chunk.vert
    shaders/chunk.frag
)

set(SPIRV_OUTPUTS "")
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPIRV_OUT "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV_OUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        COMMAND ${GLSLC_EXECUTABLE} -o ${SPIRV_OUT} ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
        COMMENT "Compiling ${SHADER_NAME} to SPIR-V"
        VERBATIM
    )
    list(APPEND SPIRV_OUTPUTS ${SPIRV_OUT})
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPIRV_OUTPUTS})

set(VOXEL_LIB_SOURCES
    src/core/Registry.cpp
    src/core/Log.cpp
    src/core/Config.cpp
    src/core/Blocks.cpp
    src/core/World.cpp
    src/gfx/VkContext.cpp
    src/gfx/Headless.cpp
    src/gfx/HeadlessChunks.cpp
    src/gfx/Camera.cpp
    src/gfx/Frustum.cpp
    src/gfx/TextureArray.cpp
    src/gfx/ChunkRenderer.cpp
    src/gfx/ChunkRendererRegister.cpp
    src/gfx/TestCubeCamera.cpp
    src/gfx/Swapchain.cpp
    src/gfx/DesktopApp.cpp
    src/gfx/scene/TestCubeScene.cpp
)

add_executable(voxel
    src/main.cpp
    ${VOXEL_LIB_SOURCES}
)

add_dependencies(voxel shaders)

target_include_directories(voxel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Stb_INCLUDE_DIR}
)

target_compile_definitions(voxel PRIVATE
    VOXEL_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders"
    VOXEL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    VK_NO_PROTOTYPES
)

target_link_libraries(voxel PRIVATE
    volk::volk
    vk-bootstrap::vk-bootstrap
    Vulkan::Headers
    GPUOpen::VulkanMemoryAllocator
    glfw
    glm::glm
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

if(WIN32)
    target_compile_definitions(voxel PRIVATE NOMINMAX)
endif()

add_executable(voxel_desktop
    src/desktop_main.cpp
    ${VOXEL_LIB_SOURCES}
)
add_dependencies(voxel_desktop shaders)
target_include_directories(voxel_desktop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Stb_INCLUDE_DIR}
)
target_compile_definitions(voxel_desktop PRIVATE
    VOXEL_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders"
    VOXEL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    VK_NO_PROTOTYPES
)
target_link_libraries(voxel_desktop PRIVATE
    volk::volk
    vk-bootstrap::vk-bootstrap
    Vulkan::Headers
    GPUOpen::VulkanMemoryAllocator
    glfw
    glm::glm
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)
if(WIN32)
    target_compile_definitions(voxel_desktop PRIVATE NOMINMAX)
endif()

add_executable(voxel_tests tests/test_main.cpp ${VOXEL_LIB_SOURCES})
add_dependencies(voxel_tests shaders)

target_include_directories(voxel_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Stb_INCLUDE_DIR}
)

target_compile_definitions(voxel_tests PRIVATE
    VOXEL_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders"
    VOXEL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    VK_NO_PROTOTYPES
)

target_link_libraries(voxel_tests PRIVATE
    doctest::doctest
    volk::volk
    vk-bootstrap::vk-bootstrap
    Vulkan::Headers
    GPUOpen::VulkanMemoryAllocator
    glfw
    glm::glm
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

enable_testing()
add_test(NAME voxel_tests COMMAND voxel_tests)
set_tests_properties(voxel_tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    ENVIRONMENT "VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json"
)
