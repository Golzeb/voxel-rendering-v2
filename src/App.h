#pragma once
#include <memory>
#include <vector>

// clang-format off
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/trigonometric.hpp"
#include "renderers/GridHierarchyRayMarchingRenderer.h"
#include "renderers/InstancedRasterRenderer.h"
#include "renderers/RasterRenderer.h"
// clang-format on

#include "GLFW/glfw3.h"
#include "VoxelData.h"
#include "glm/ext/vector_float2.hpp"
#include "masks/IMask.h"
#include "renderers/IRenderer.h"
#include "renderers/RayMarchingRenderer.h"
#include "transfer_functions/ITransferFunction.h"

constexpr float BASE_CAMERA_SPEED = 40.0f;

enum class Renderer : int {
    NOOP = 0,
    RASTER,
    INSTANCED_RASTER,
    RAY_MARCHING,
    GRID_HIERARCHY_RAY_MARCHING,
    BRICKMAP_RAY_MARCHING,
    COUNT,
};

struct ApplicationState {
    GLFWwindow* window;

    VoxelData data;
    glm::mat4 model = glm::identity<glm::mat4>();
    glm::vec3 model_position = {0.0f, 0.0f, 0.0f};
    glm::vec3 model_origin = {};
    glm::vec3 model_rotation = {0.0f, 0.0f, 0.0f};
    glm::vec3 model_scale = {1.0f, 1.0f, 1.0f};

    bool mouse_locked = true;
    glm::vec2 previous_mouse_position = glm::vec2(0.0f);

    Renderer selected_renderer = Renderer::NOOP;
    std::vector<char const*> renderer_names{"Noop renderer",
                                            "Raster renderer",
                                            "Instanced raster renderer",
                                            "Ray marching renderer",
                                            "Grid hierarchy ray marching renderer",
                                            "Brickmap ray marching renderer"};

    bool back_face_culling = false;
    bool wireframe = false;

    RasterRenderer::Mode raster_renderer_mode;
    InstancedRasterRenderer::Mode instanced_raster_renderer_mode;

    RayMarchingRenderer::Mode ray_marching_renderer_mode;
    GridHierarchyRayMarchingRenderer::Mode grid_hierarchy_ray_marching_renderer_mode;

    std::shared_ptr<IRenderer> current_renderer{};
    std::shared_ptr<ITransferFunction> transfer_function{};
    std::shared_ptr<IMask> mask{};

    float camera_speed = BASE_CAMERA_SPEED;

    float delta_time = 0.0f;
    float gpu_time = 0.0f;

    float load_time = 0.0f;

    bool draw_controls = false;
};

inline void generate_model_matrix(ApplicationState& appstate) {
    glm::quat const rotation = glm::quat(glm::radians(appstate.model_rotation));

    appstate.model = glm::identity<glm::mat4>();
    appstate.model = glm::translate(appstate.model, appstate.model_origin);
    appstate.model = appstate.model * glm::mat4_cast(rotation);
    appstate.model = glm::translate(appstate.model, -appstate.model_origin);
    appstate.model = glm::translate(appstate.model, appstate.model_origin);
    appstate.model = glm::scale(appstate.model, appstate.model_scale);
    appstate.model = glm::translate(appstate.model, -appstate.model_origin);
    appstate.model = glm::translate(appstate.model, appstate.model_position);
}
