#include "utils.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

// clang-format off
#include "glad/gl.h"
#include "GLFW/glfw3.h"
// clang-format on

#include "glm/ext/vector_float2.hpp"
#include "spdlog/spdlog.h"

namespace utils {
std::string load_shader(std::string_view path) {
    std::ifstream file;
    file.open(path.data());

    std::stringstream ss;
    ss << file.rdbuf();

    file.close();

    return ss.str();
}

std::string human_readable_size(std::size_t size) {
    constexpr std::array<std::size_t, 3> SIZES = {1024, 1024 * 1024, 1024 * 1024 * 1024};

    if (size > SIZES[2]) {
        return std::format("{} GiB", size / float(SIZES[2]));
    } else if (size > SIZES[1]) {
        return std::format("{} MiB", size / float(SIZES[1]));
    } else if (size > SIZES[0]) {
        return std::format("{} KiB", size / float(SIZES[0]));
    } else {
        return std::format("{} B", size);
    }
}

void check_shader_compilation_errors(GLuint shader_name) {
    std::int32_t read_length;

    std::array<char, 2048> info_log{};
    glGetShaderInfoLog(shader_name, info_log.size(), &read_length, info_log.data());

    std::int32_t success;
    glGetShaderiv(shader_name, GL_COMPILE_STATUS, &success);

    if (!success) {
        spdlog::error("Shader compilation error: {}", info_log.data());
    }
}

glm::vec2 get_cursor_position(GLFWwindow* window) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    return glm::vec2{x, y};
}

std::chrono::time_point<std::chrono::steady_clock> time_start() {
    static std::chrono::steady_clock clock{};
    return clock.now();
}

float time_count(std::chrono::time_point<std::chrono::steady_clock> const& start) {
    return std::chrono::duration<float>(time_start() - start).count();
}
}  // namespace utils
