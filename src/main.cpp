#include <memory>
#include <utility>
#include <vector>

// clang-format off
#include "glad/gl.h"
#include "GLFW/glfw3.h"
//clang-format on

#include "config.h"
#include "renderers/IRenderer.h"
#include "renderers/NoopRenderer.h"
#include "spdlog/spdlog.h"

GLFWwindow* create_window();
void populate_renderers(std::vector<std::shared_ptr<IRenderer>>& renderers, std::vector<const char*>& names);

enum class Renderer : int {
    NOOP = 0,
    COUNT,
};

struct ApplicationState {
    Renderer current_renderer;

};

int main() {
    glfwInit();

    GLFWwindow* window = create_window();

    ApplicationState appstate{
        .current_renderer = Renderer::NOOP,
    };

    std::vector<std::shared_ptr<IRenderer>> renderers;
    std::vector<const char*> renderer_names;
    populate_renderers(renderers, renderer_names);

    std::shared_ptr<IRenderer> current_renderer = renderers[std::to_underlying(appstate.current_renderer)];

    while (!glfwWindowShouldClose(window)) {
        current_renderer->clear();
        current_renderer->render();
        current_renderer->update();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
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
}
