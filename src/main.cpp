#include <memory>
#include <utility>
#include <vector>

// clang-format off
#include "glad/gl.h"
#include "GLFW/glfw3.h"
//clang-format on

#include "glm/trigonometric.hpp"
#include "transfer_functions/ITransferFunction.h"
#include "utils.h"
#include "Camera.h"
#include "config.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"
#include "loaders.h"
#include "masks/ThresholdMask.h"
#include "renderers/IRenderer.h"
#include "renderers/NaiveRasterRenderer.h"
#include "renderers/NoopRenderer.h"
#include "spdlog/spdlog.h"

enum class Renderer : int {
    NOOP = 0,
    NAIVE_RASTER,
    COUNT,
};

struct ApplicationState {
    GLFWwindow* window;

    bool mouse_locked;
    glm::vec2 previous_mouse_position;

    Renderer current_renderer;
    std::vector<std::shared_ptr<IRenderer>> renderers;
    std::vector<const char*> renderer_names;

    std::unique_ptr<ITransferFunction> transfer_function;

    float camera_speed;
};

GLFWwindow* create_window();
void populate_renderers(std::vector<std::shared_ptr<IRenderer>>& renderers, std::vector<const char*>& names);
void handle_camera_input(ApplicationState& appstate, Camera& camera, float delta_time); 
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main() {
    glfwInit();

    ApplicationState appstate{
        .window = create_window(),
        .mouse_locked = true,
        .previous_mouse_position = glm::vec2(0.0f),
        
        .current_renderer = Renderer::NOOP,
        .renderers = {},
        .renderer_names = {},

        .camera_speed = 40.0f,
    };
    glfwSetWindowUserPointer(appstate.window, &appstate);
    appstate.previous_mouse_position = utils::get_cursor_position(appstate.window);

    appstate.current_renderer = Renderer::NAIVE_RASTER;

    populate_renderers(appstate.renderers, appstate.renderer_names);

    std::shared_ptr<IRenderer> current_renderer = appstate.renderers[std::to_underlying(appstate.current_renderer)];

    auto [data, transfer_function] = loaders::read_vl32("data/bistro_512x512x512.vl32", glm::ivec3{512, 512, 512});

    Camera camera (glm::perspective(glm::radians(90.0f), float(IMAGE_WIDTH) / float(IMAGE_HEIGHT), 0.01f, 2000.0f), glm::vec2{-89.0, 89.0});

    current_renderer->init();
    current_renderer->update_data(data, transfer_function, ThresholdMask(0, true));

    float delta_time = 0.0f; 
    while (!glfwWindowShouldClose(appstate.window)) {
        float const frame_start = glfwGetTime();

        handle_camera_input(appstate, camera, delta_time);
        camera.update();

        current_renderer->clear();
        current_renderer->render(camera);
        current_renderer->update();

        glfwSwapBuffers(appstate.window);
        glfwPollEvents();

        delta_time = glfwGetTime() - frame_start;
    }

    glfwDestroyWindow(appstate.window);
    glfwTerminate();
}

GLFWwindow* create_window() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(IMAGE_WIDTH, IMAGE_HEIGHT, "Voxel Rendering", nullptr, nullptr);

    glfwMakeContextCurrent(window);
    glfwShowWindow(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    int version = gladLoadGL(glfwGetProcAddress);

    spdlog::info("Loaded OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    glfwSwapInterval(1);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glViewport(0, 0, IMAGE_WIDTH, IMAGE_HEIGHT);

    return window;
}

void populate_renderers(std::vector<std::shared_ptr<IRenderer>>& renderers, std::vector<const char*>& names) {
    renderers.resize(std::to_underlying(Renderer::COUNT), {});
    names.resize(std::to_underlying(Renderer::COUNT), nullptr);

    renderers[std::to_underlying(Renderer::NOOP)] = std::make_shared<NoopRenderer>();
    names[std::to_underlying(Renderer::NOOP)] = renderers[std::to_underlying(Renderer::NOOP)]->name();

    renderers[std::to_underlying(Renderer::NAIVE_RASTER)] = std::make_shared<NaiveRasterRenderer>();
    names[std::to_underlying(Renderer::NAIVE_RASTER)] = renderers[std::to_underlying(Renderer::NAIVE_RASTER)]->name();
}

void handle_camera_input(ApplicationState& appstate, Camera& camera, float delta_time) {
    if (appstate.mouse_locked) {
        glm::vec2 current_mouse_position = utils::get_cursor_position(appstate.window);
        glm::vec2 mouse_delta = current_mouse_position - appstate.previous_mouse_position;
        appstate.previous_mouse_position = current_mouse_position;
        camera.update_pitch(mouse_delta.y * 0.1f);
        camera.update_yaw(-mouse_delta.x * 0.1f);
    }

    if (glfwGetKey(appstate.window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.update_position(camera.forward() * appstate.camera_speed * delta_time);
    }

    if (glfwGetKey(appstate.window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.update_position(-camera.forward() * appstate.camera_speed * delta_time);
    }

    if (glfwGetKey(appstate.window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.update_position(-camera.right() * appstate.camera_speed * delta_time);
    }

    if (glfwGetKey(appstate.window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.update_position(camera.right() * appstate.camera_speed * delta_time);
    }

    if (glfwGetKey(appstate.window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.update_position(glm::vec3(0.0f, 1.0f, 0.0f) * appstate.camera_speed * delta_time);
    }

    if (glfwGetKey(appstate.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera.update_position(glm::vec3(0.0f, -1.0f, 0.0f) * appstate.camera_speed * delta_time);
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ApplicationState* appstate = (ApplicationState*)glfwGetWindowUserPointer(window);
    if(key == GLFW_KEY_F && action == GLFW_PRESS) {
        appstate->mouse_locked = !appstate->mouse_locked;
        if (appstate->mouse_locked) {
            appstate->previous_mouse_position = utils::get_cursor_position(window);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    if(key == GLFW_KEY_Q && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}
