#pragma once
#include <memory>
#include <vector>

// clang-format off
#include "renderers/RasterRenderer.h"
// clang-format on

#include "GLFW/glfw3.h"
#include "VoxelData.h"
#include "glm/ext/vector_float2.hpp"
#include "masks/IMask.h"
#include "renderers/IRenderer.h"
#include "transfer_functions/ITransferFunction.h"

enum class Renderer : int {
    NOOP = 0,
    RASTER,
    COUNT,
};

struct ApplicationState {
    GLFWwindow* window;

    VoxelData data;

    bool mouse_locked = true;
    glm::vec2 previous_mouse_position = glm::vec2(0.0f);

    Renderer selected_renderer = Renderer::NOOP;
    std::vector<char const*> renderer_names{"Noop renderer", "Raster rendrer"};
    RasterRenderer::Mode raster_renderer_mode;
    bool back_face_culling = false;

    std::shared_ptr<IRenderer> current_renderer{};
    std::shared_ptr<ITransferFunction> transfer_function{};
    std::shared_ptr<IMask> mask{};

    float camera_speed = 40.0f;

    float delta_time = 0.0f;
    float gpu_time = 0.0f;

    bool draw_controls = false;
};
