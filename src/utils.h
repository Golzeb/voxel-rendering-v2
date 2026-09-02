#pragma once
#include <cstddef>
#include <string>
#include <string_view>

// clang-format off
#include "glad/gl.h"
#include "GLFW/glfw3.h"
// clang-format on

#include "glm/ext/vector_float2.hpp"
#include "taskflow/core/executor.hpp"

namespace utils {
[[nodiscard]]
std::string load_shader(std::string_view path);

[[nodiscard]]
std::string human_readable_size(std::size_t size);

void check_shader_compilation_errors(GLuint shader_name);

glm::vec2 get_cursor_position(GLFWwindow* window);
}  // namespace utils
