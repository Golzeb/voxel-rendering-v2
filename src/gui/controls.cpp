#include "controls.h"

#include <memory>

// clang-format off
#include "allocators/TrackingAllocator.h"
// clang-format on

#include "App.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "renderers/NoopRenderer.h"
#include "renderers/RasterRenderer.h"
#include "spdlog/spdlog.h"
#include "utils.h"

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

    ImGui::Dummy({0, 10});

    ImGui::Combo("Renderer", (int*)&appstate.selected_renderer, appstate.renderer_names.data(),
                 appstate.renderer_names.size());
    if (ImGui::Checkbox("Back face culling", &appstate.back_face_culling)) {
        if (appstate.back_face_culling) {
            glEnable(GL_CULL_FACE);
        } else {
            glDisable(GL_CULL_FACE);
        }
    }

    if (appstate.selected_renderer == Renderer::RASTER) {
        ImGui::Checkbox("Indexed", &appstate.raster_renderer_mode.indexed);
        ImGui::Checkbox("Chunked", &appstate.raster_renderer_mode.chunked);
        if (appstate.raster_renderer_mode.chunked) {
            ImGui::SliderInt("Chunk size", &appstate.raster_renderer_mode.chunk_size, 0, 512);
        }

        if (!appstate.raster_renderer_mode.greedy_mesh) {
            ImGui::Checkbox("Face culled", &appstate.raster_renderer_mode.face_culled);
        }
        if (!appstate.raster_renderer_mode.face_culled) {
            ImGui::Checkbox("Greedy mesh", &appstate.raster_renderer_mode.greedy_mesh);
        }
    }

    if (ImGui::Button("Update renderer")) {
        appstate.current_renderer->deinit();

        switch (appstate.selected_renderer) {
            case Renderer::NOOP:
                appstate.current_renderer = std::make_shared<NoopRenderer>();
                break;
            case Renderer::RASTER:
                appstate.current_renderer = std::make_shared<RasterRenderer>(appstate.raster_renderer_mode);
                break;
            case Renderer::COUNT:
                break;
        }

        talloc::reset();

        appstate.current_renderer->init();
        appstate.current_renderer->update_data(appstate.data, appstate.transfer_function, *appstate.mask);

        spdlog::info("Loaded {}", appstate.current_renderer->name());
        spdlog::info("Used RAM: {}", utils::human_readable_size(talloc::max_memory()));
        spdlog::info("Used VRAM: {}", utils::human_readable_size(talloc::current_gpu_memory()));
    }

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
