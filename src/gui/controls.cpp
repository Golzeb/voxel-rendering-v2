#include "controls.h"

#include "App.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace gui {
void init(GLFWwindow* window) {
    ImGuiContext* context = ImGui::CreateContext();
    ImGui::SetCurrentContext(context);
    ImGui_ImplOpenGL3_Init();
    ImGui_ImplGlfw_InitForOpenGL(window, true);

    // HACK: Find better solution for this (ImGui updates the cursor from hidden to visible)
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
}

void render(ApplicationState& appstate) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Controls");

    ImGui::LabelText("Average frame time (CPU)", "%.04fms", (appstate.delta_time - appstate.gpu_time) * 1000.0f);
    ImGui::LabelText("Average frame time (GPU)", "%.04fms", appstate.gpu_time * 1000.0f);
    ImGui::LabelText("Average frame time (full)", "%.04fms", appstate.delta_time * 1000.0f);

    ImGui::End();

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void deinit() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(ImGui::GetCurrentContext());
}
}  // namespace gui
