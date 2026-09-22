#pragma once
#include <memory>
#include <vector>

// clang-format off
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
    COUNT,
};

struct ApplicationState {
    GLFWwindow* window;

    VoxelData data;

    bool mouse_locked = true;
    glm::vec2 previous_mouse_position = glm::vec2(0.0f);

    Renderer selected_renderer = Renderer::NOOP;
    std::vector<char const*> renderer_names{"Noop renderer", "Raster renderer", "Instanced raster renderer",
                                            "Ray marching renderer", "Grid hierarchy ray marching renderer"};

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
