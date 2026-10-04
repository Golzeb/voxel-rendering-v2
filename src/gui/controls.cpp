#include "controls.h"

#include <array>
#include <cstddef>
#include <memory>
#include <numeric>

// clang-format off
#include "allocators/TrackingAllocator.h"
// clang-format on

#include "App.h"
#include "GLFW/glfw3.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "renderers/BrickmapRayMarchingRenderer.h"
#include "renderers/GridHierarchyRayMarchingRenderer.h"
#include "renderers/InstancedRasterRenderer.h"
#include "renderers/NoopRenderer.h"
#include "renderers/RasterRenderer.h"
#include "renderers/RayMarchingRenderer.h"
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
    static bool colour_controls = false;

    static bool model_controls = false;
    static glm::mat4 model = {};
    static glm::vec3 model_position = {};
    static glm::vec3 model_origin = {};
    static glm::vec3 model_rotation = {};
    static glm::vec3 model_scale = {1.0, 1.0, 1.0};

    static std::array<float, 120> gpu_time_collect;
    static std::size_t gpu_time_index = 0;

    static std::array<float, 120> cpu_time_collect;
    static std::size_t cpu_time_index = 0;

    static std::array<float, 120> delta_time_collect;
    static std::size_t delta_time_index = 0;

    gpu_time_collect[gpu_time_index++ % 120] = appstate.gpu_time;
    cpu_time_collect[cpu_time_index++ % 120] = appstate.delta_time - appstate.gpu_time;
    delta_time_collect[delta_time_index++ % 120] = appstate.delta_time;

    float const cpu_time = std::reduce(cpu_time_collect.begin(), cpu_time_collect.end()) / 120.0f * 1000.0f;
    float const gpu_time = std::reduce(gpu_time_collect.begin(), gpu_time_collect.end()) / 120.0f * 1000.0f;
    float const delta_time = std::reduce(delta_time_collect.begin(), delta_time_collect.end()) / 120.0f * 1000.0f;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Controls");

    ImGui::LabelText("Frames per second", "%.02f", 1000.0f / delta_time);
    ImGui::LabelText("Average frame time (CPU)", "%.04fms", cpu_time);
    ImGui::LabelText("Average frame time (GPU)", "%.04fms", gpu_time);
    ImGui::LabelText("Average frame time (full)", "%.04fms", delta_time);

    ImGui::Dummy({0, 10});

    if (ImGui::Checkbox("Back face culling", &appstate.back_face_culling)) {
        if (appstate.back_face_culling) {
            glEnable(GL_CULL_FACE);
        } else {
            glDisable(GL_CULL_FACE);
        }
    }

    if (ImGui::Checkbox("Wireframe", &appstate.wireframe)) {
        if (appstate.wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    ImGui::Dummy({0, 10});

    ImGui::Combo("Renderer", (int*)&appstate.selected_renderer, appstate.renderer_names.data(),
                 appstate.renderer_names.size());

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
    } else if (appstate.selected_renderer == Renderer::INSTANCED_RASTER) {
        ImGui::Checkbox("Indexed", &appstate.instanced_raster_renderer_mode.indexed);
        ImGui::Checkbox("Face culled", &appstate.instanced_raster_renderer_mode.face_culled);
    }

    if (appstate.selected_renderer == Renderer::RAY_MARCHING) {
        ImGui::Checkbox("Chunked", &appstate.ray_marching_renderer_mode.chunked);

        if (appstate.ray_marching_renderer_mode.chunked) {
            ImGui::SliderInt("Chunk size", &appstate.ray_marching_renderer_mode.chunk_size, 0, 512);

            ImGui::Checkbox("Sorted", &appstate.ray_marching_renderer_mode.sorted);
        }
    }

    if (appstate.selected_renderer == Renderer::GRID_HIERARCHY_RAY_MARCHING) {
        ImGui::Checkbox("Chunked", &appstate.grid_hierarchy_ray_marching_renderer_mode.chunked);

        if (appstate.grid_hierarchy_ray_marching_renderer_mode.chunked) {
            ImGui::SliderInt("Chunk size", &appstate.grid_hierarchy_ray_marching_renderer_mode.chunk_size, 0, 2048);

            ImGui::Checkbox("Sorted", &appstate.grid_hierarchy_ray_marching_renderer_mode.sorted);
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
            case Renderer::INSTANCED_RASTER:
                appstate.current_renderer =
                    std::make_shared<InstancedRasterRenderer>(appstate.instanced_raster_renderer_mode);
                break;
            case Renderer::RAY_MARCHING:
                appstate.current_renderer = std::make_shared<RayMarchingRenderer>(appstate.ray_marching_renderer_mode);
                break;
            case Renderer::GRID_HIERARCHY_RAY_MARCHING:
                appstate.current_renderer = std::make_shared<GridHierarchyRayMarchingRenderer>(
                    appstate.grid_hierarchy_ray_marching_renderer_mode);
                break;
            case Renderer::BRICKMAP_RAY_MARCHING:
                appstate.current_renderer = std::make_shared<BrickmapRayMarchingRenderer>();
                break;
            case Renderer::COUNT:
                break;
        }

        talloc::reset();

        appstate.current_renderer->init();

        auto start = utils::time_start();
        appstate.current_renderer->update_data(appstate.data, appstate.transfer_function, appstate.mask);
        appstate.load_time = utils::time_count(start);
    }

    ImGui::Dummy({0, 10});

    ImGui::LabelText("Peak RAM", "%s", utils::human_readable_size(talloc::max_memory()).c_str());
    ImGui::LabelText("Used VRAM", "%s", utils::human_readable_size(talloc::current_gpu_memory()).c_str());
    ImGui::LabelText("Load time", "%.04fms", appstate.load_time);

    ImGui::Dummy({0, 10});

    ImGui::Checkbox("Colour controls", &colour_controls);

    if (colour_controls) {
        ImGui::Begin("Colour mapping");

        if (appstate.transfer_function->controls()) {
            appstate.current_renderer->update_colour(appstate.transfer_function);
        }

        ImGui::End();
    }

    ImGui::Checkbox("Model controls", &model_controls);

    if (model_controls) {
        ImGui::Begin("Model");

        if (ImGui::InputFloat3("Position", glm::value_ptr(appstate.model_position)) ||
            ImGui::InputFloat3("Rotation", glm::value_ptr(appstate.model_rotation)) ||
            ImGui::InputFloat3("Scale", glm::value_ptr(appstate.model_scale)) ||
            ImGui::InputFloat3("Origin", glm::value_ptr(appstate.model_origin))) {
            generate_model_matrix(appstate);
        }

        ImGui::End();
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
