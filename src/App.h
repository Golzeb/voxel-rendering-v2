#pragma once
#include <memory>
#include <vector>

#include "GLFW/glfw3.h"
#include "glm/ext/vector_float2.hpp"
#include "renderers/IRenderer.h"

enum class Renderer : int {
    NOOP = 0,
    NAIVE_RASTER,
    INDEXED_RASTER,
    COUNT,
};

struct ApplicationState {
    GLFWwindow* window;

    bool mouse_locked = true;
    glm::vec2 previous_mouse_position = glm::vec2(0.0f);

    Renderer current_renderer = Renderer::NOOP;
    std::vector<std::shared_ptr<IRenderer>> renderers{};
    std::vector<char const*> renderer_names{};

    float camera_speed = 40.0f;

    float delta_time = 0.0f;
    float gpu_time = 0.0f;

    bool draw_controls = false;
};
