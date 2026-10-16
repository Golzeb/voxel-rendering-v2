#pragma once
#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>

// clang-format off
#include "glad/gl.h"
#include "GLFW/glfw3.h"
// clang-format on

#include "glm/ext/vector_float2.hpp"

namespace utils {
using time_point_t = std::chrono::time_point<std::chrono::steady_clock>;
[[nodiscard]]
std::string load_shader(std::string_view path);

[[nodiscard]]
std::string human_readable_size(std::size_t size);

void check_shader_compilation_errors(GLuint shader_name);

glm::vec2 get_cursor_position(GLFWwindow* window);

[[nodiscard]] time_point_t time_start();

[[nodiscard]]
float time_count(time_point_t const& start);
}  // namespace utils
