#include <memory>

// clang-format off
#include "glad/gl.h"
#include "GLFW/glfw3.h"
//clang-format on

#include "glm/trigonometric.hpp"
#include "gui/controls.h"
#include "imgui.h"
#include "utils.h"
#include "Camera.h"
#include "config.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "renderers/NoopRenderer.h"
#include "spdlog/spdlog.h"
#include "App.h"

GLFWwindow* create_window();
void handle_camera_input(ApplicationState& appstate, Camera& camera); 
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main() {
    glfwInit();

    ApplicationState appstate{
        .window = create_window(),
        .mouse_locked = true,
        .selected_renderer = Renderer::NOOP,
        .camera_speed = 40.0f,
        .draw_controls = true,
    };

    gui::init(appstate.window);

    glfwSetWindowUserPointer(appstate.window, &appstate);
    appstate.previous_mouse_position = utils::get_cursor_position(appstate.window);

    appstate.selected_renderer = Renderer::NOOP;
    appstate.current_renderer = std::make_shared<NoopRenderer>();

    auto [data, mask, transfer_function, transform] = appstate.scene_loader.get(0);

    appstate.data = data;
    appstate.mask = mask;
    appstate.transfer_function = transfer_function;
    appstate.model_origin = transform.origin;
    appstate.model_position = transform.position;
    appstate.model_rotation = transform.rotation;
    appstate.model_scale = transform.scale;

    generate_model_matrix(appstate);

    Camera camera (glm::perspective(glm::radians(90.0f), float(IMAGE_WIDTH) / float(IMAGE_HEIGHT), 0.1f, 2000.0f), glm::vec2{-89.0, 89.0});

    GLuint query;
    glGenQueries(1, &query);

    while (!glfwWindowShouldClose(appstate.window)) {
        float const frame_start = glfwGetTime();

        if(!appstate.benchmark.is_running()) {
            handle_camera_input(appstate, camera);
            camera.update();
        } else {
            appstate.benchmark.run(camera);
        }

        camera.set_model(appstate.model);

        glFinish();
        glBeginQuery(GL_TIME_ELAPSED, query);

        appstate.current_renderer->clear();
        appstate.current_renderer->render(camera);
        
        glFinish();
        glEndQuery(GL_TIME_ELAPSED);

        if(appstate.draw_controls) {
            gui::render(appstate);
        }

        glfwSwapBuffers(appstate.window);
        glfwPollEvents();

        appstate.delta_time = glfwGetTime() - frame_start;

        GLint64 gpu_time_elapsed;
        glGetQueryObjecti64v(query, GL_QUERY_RESULT, &gpu_time_elapsed);

        appstate.gpu_time = gpu_time_elapsed * 1e-9f;
    }

    gui::deinit();
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

void handle_camera_input(ApplicationState& appstate, Camera& camera) {
    appstate.camera_speed = BASE_CAMERA_SPEED; 
    if (appstate.mouse_locked) {
        glm::vec2 current_mouse_position = utils::get_cursor_position(appstate.window);
        glm::vec2 mouse_delta = current_mouse_position - appstate.previous_mouse_position;
        appstate.previous_mouse_position = current_mouse_position;
        camera.update_pitch(mouse_delta.y * 0.1f);
        camera.update_yaw(-mouse_delta.x * 0.1f);
    }

    if(glfwGetKey(appstate.window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        appstate.camera_speed = BASE_CAMERA_SPEED * 2.0f;
    }

    if (glfwGetKey(appstate.window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.update_position(camera.forward() * appstate.camera_speed * appstate.delta_time);
    }

    if (glfwGetKey(appstate.window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.update_position(-camera.forward() * appstate.camera_speed * appstate.delta_time);
    }

    if (glfwGetKey(appstate.window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.update_position(-camera.right() * appstate.camera_speed * appstate.delta_time);
    }

    if (glfwGetKey(appstate.window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.update_position(camera.right() * appstate.camera_speed * appstate.delta_time);
    }

    if (glfwGetKey(appstate.window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.update_position(glm::vec3(0.0f, 1.0f, 0.0f) * appstate.camera_speed * appstate.delta_time);
    }

    if (glfwGetKey(appstate.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera.update_position(glm::vec3(0.0f, -1.0f, 0.0f) * appstate.camera_speed * appstate.delta_time);
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

        ImGui::GetIO().WantSetMousePos = !appstate->mouse_locked;
    }

    if(key == GLFW_KEY_Q && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if(key == GLFW_KEY_G && action == GLFW_PRESS) {
        appstate->draw_controls = !appstate->draw_controls;
    }
}
