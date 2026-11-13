#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>

// clang-format off
#include "glad/gl.h"
#include "GLFW/glfw3.h"
// clang-format on

#include <glaze/core/context.hpp>
#include <glaze/core/istream_buffer.hpp>
#include <glaze/forward.hpp>
#include <glaze/json/read.hpp>
#include <glaze/json/write.hpp>
#include <utility>
#include <variant>

#include "App.h"
#include "Camera.h"
#include "config.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"
#include "gui/controls.h"
#include "imgui.h"
#include "renderers/BrickmapRayMarchingRenderer.h"
#include "renderers/GridHierarchyRayMarchingRenderer.h"
#include "renderers/InstancedRasterRenderer.h"
#include "renderers/NoopRenderer.h"
#include "renderers/RasterRenderer.h"
#include "renderers/RayMarchingRenderer.h"
#include "renderers/SVORayMarchingRenderer.h"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include "utils.h"

using namespace std::string_view_literals;

struct EmptyMode {};

struct Test {
    std::string renderer;

    std::string scene_name;
    std::int32_t scene_size = 0;
    bool noise = false;

    bool backface_culling = false;

    std::variant<RasterRenderer::Mode, RayMarchingRenderer::Mode, InstancedRasterRenderer::Mode,
                 GridHierarchyRayMarchingRenderer::Mode, EmptyMode>
        mode;
};

GLFWwindow* create_window();
void handle_camera_input(ApplicationState& appstate, Camera& camera);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main(int argc, char* argv[]) {
    glfwInit();

    spdlog::set_level(spdlog::level::debug);

    Test test;

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

    std::string output_path;

    std::int32_t scene_id = 0;
    if (argc > 1) {
        std::ifstream file(argv[1]);
        glz::basic_istream_buffer<std::ifstream> buffer(file);
        std::filesystem::path file_path = std::filesystem::path(argv[1]);

        std::filesystem::path temp = file_path.relative_path();
        if (!temp.empty()) file_path = file_path.lexically_relative(*temp.begin());
        output_path = file_path;

        spdlog::debug("Output filename: {}", output_path);

        auto ec = glz::read_json<Test>(test, buffer);

        if (ec) {
            spdlog::error("{} (code: {})", ec.custom_error_message, std::to_underlying(ec.ec));
        }

        appstate.draw_controls = false;
        appstate.benchmark_mode = true;
        appstate.backface_culling = test.backface_culling;

        if (appstate.backface_culling) {
            glEnable(GL_CULL_FACE);
        } else {
            glDisable(GL_CULL_FACE);
        }

        scene_id = appstate.scene_loader.get_id_by_name(test.scene_name);

        if (test.renderer == "raster"sv) {
            appstate.selected_renderer = Renderer::RASTER;
            appstate.raster_renderer_mode = std::get<RasterRenderer::Mode>(test.mode);
            appstate.current_renderer = std::make_shared<RasterRenderer>(appstate.raster_renderer_mode);
        } else if (test.renderer == "raymarch"sv) {
            appstate.selected_renderer = Renderer::RAY_MARCHING;
            appstate.ray_marching_renderer_mode = std::get<RayMarchingRenderer::Mode>(test.mode);
            appstate.current_renderer = std::make_shared<RayMarchingRenderer>(appstate.ray_marching_renderer_mode);
        } else if (test.renderer == "instanced"sv) {
            appstate.selected_renderer = Renderer::INSTANCED_RASTER;
            appstate.instanced_raster_renderer_mode = std::get<InstancedRasterRenderer::Mode>(test.mode);
            appstate.current_renderer =
                std::make_shared<InstancedRasterRenderer>(appstate.instanced_raster_renderer_mode);
        } else if (test.renderer == "grid"sv) {
            appstate.selected_renderer = Renderer::GRID_HIERARCHY_RAY_MARCHING;
            appstate.grid_hierarchy_ray_marching_renderer_mode =
                std::get<GridHierarchyRayMarchingRenderer::Mode>(test.mode);
            appstate.current_renderer =
                std::make_shared<GridHierarchyRayMarchingRenderer>(appstate.grid_hierarchy_ray_marching_renderer_mode);
        } else if (test.renderer == "brickmap"sv) {
            appstate.selected_renderer = Renderer::BRICKMAP_RAY_MARCHING;
            appstate.current_renderer = std::make_shared<BrickmapRayMarchingRenderer>();
        } else if (test.renderer == "svo"sv) {
            appstate.selected_renderer = Renderer::SVO_RAY_MARCHING;
            appstate.current_renderer = std::make_shared<SVORayMarchingRenderer>();
        }
    }

    appstate.current_renderer->init();

    auto [data, mask, transfer_function, transform, points] =
        appstate.scene_loader.get(scene_id, test.scene_size, test.noise);

    appstate.benchmark_points = points.points;
    appstate.benchmark_scale = points.scale;
    appstate.data = data;
    appstate.mask = mask;
    appstate.transfer_function = transfer_function;
    appstate.model_origin = transform.origin;
    appstate.model_position = transform.position;
    appstate.model_rotation = transform.rotation;
    appstate.model_scale = transform.scale;

    generate_model_matrix(appstate);
    if (argc > 1) {
        appstate.benchmark.start(appstate, points.points, points.scale);
    }

    Camera camera(glm::perspective(glm::radians(90.0f), float(IMAGE_WIDTH) / float(IMAGE_HEIGHT), 0.1f, 2000.0f),
                  glm::vec2{-89.0, 89.0});

    appstate.camera = &camera;

    GLuint query;
    glGenQueries(1, &query);

    while (!glfwWindowShouldClose(appstate.window)) {
        float const frame_start = glfwGetTime();

        if (!appstate.benchmark.is_running()) {
            if (appstate.benchmark_mode) {
                glfwSetWindowShouldClose(appstate.window, 1);
                std::string buffer = glz::write_json(appstate.benchmark.stats()).value_or("error");
                std::filesystem::path out_path = std::format("tests_out/{}", output_path);
                if (!std::filesystem::exists(out_path)) {
                    std::filesystem::create_directories(std::filesystem::path(out_path).remove_filename());
                }
                spdlog::info("{}", out_path.c_str());
                std::ofstream out(out_path);
                out.write(buffer.c_str(), buffer.size());
                out.close();
            }

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

        if (appstate.draw_controls) {
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

    glfwSwapInterval(0);

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

    if (glfwGetKey(appstate.window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
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
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        appstate->mouse_locked = !appstate->mouse_locked;

        if (appstate->mouse_locked) {
            appstate->previous_mouse_position = utils::get_cursor_position(window);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        ImGui::GetIO().WantSetMousePos = !appstate->mouse_locked;
    }

    if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        appstate->draw_controls = !appstate->draw_controls;
    }
}
