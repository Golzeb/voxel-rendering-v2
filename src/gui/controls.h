#pragma once

#include "App.h"
#include "GLFW/glfw3.h"

namespace gui {
void init(GLFWwindow* window);
void render(ApplicationState& state);
void deinit();
}  // namespace gui
