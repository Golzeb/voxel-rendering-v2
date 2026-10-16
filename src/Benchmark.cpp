#include "Benchmark.h"

#include <cmath>
#include <cstddef>
#include <vector>

#include "App.h"
#include "Camera.h"
#include "allocators/TrackingAllocator.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"
#include "spdlog/spdlog.h"
#include "utils.h"

void Benchmark::start(ApplicationState& appstate, std::vector<Point> const& points) {
    constexpr std::size_t ITERS = 5;
    m_running = true;
    m_points = points;
    m_current_frame = 0;
    m_frame_total = 0;
    m_index = 0;

    auto const start = utils::time_start();
    for (std::size_t i = 0; i < ITERS; i++) {
        appstate.current_renderer->update_data(appstate.data, appstate.transfer_function, appstate.mask);
    }
    float const load_time = utils::time_count(start);

    m_stats.average_load_time = load_time / float(ITERS);
    m_stats.ram_usage = talloc::max_memory();
    m_stats.vram_usage = talloc::current_gpu_memory();

    m_start = utils::time_start();
}

void Benchmark::run(Camera& camera) {
    if (!m_running) {
        return;
    }

    m_frame_total += 1;

    if (m_current_frame < m_points[m_index].length) {
        if (m_index == 0 && m_current_frame == 0) {
            camera.set_position(m_points[m_index].position);
            glm::vec3 const diff = glm::normalize(m_points[m_index].look_at - camera.position());

            float const pitch = glm::degrees(std::asin(-diff.y));
            float const yaw = glm::degrees(std::atan2(diff.x, diff.z));

            camera.set_pitch(pitch);
            camera.set_yaw(yaw);
            camera.update();
        }

        m_current_frame += 1;
        return;
    }

    m_index += 1;
    if (m_index >= m_points.size()) {
        float const time = utils::time_count(m_start);

        m_stats.average_frame_time = time / float(m_frame_total);

        m_running = false;

        spdlog::info("Average frame time: {}ms", m_stats.average_frame_time * 1000.0f);
        spdlog::info("Average load time: {}ms", m_stats.average_load_time * 1000.0f);
        spdlog::info("RAM usage: {}", utils::human_readable_size(m_stats.ram_usage));
        spdlog::info("VRAM usage: {}", utils::human_readable_size(m_stats.vram_usage));

        return;
    }

    m_current_frame = 0;
    camera.set_position(m_points[m_index].position);
    glm::vec3 const diff = glm::normalize(m_points[m_index].look_at - camera.position());

    float const pitch = glm::degrees(std::asin(-diff.y));
    float const yaw = glm::degrees(std::atan2(diff.x, diff.z));

    camera.set_pitch(pitch);
    camera.set_yaw(yaw);
    camera.update();
}

bool Benchmark::is_running() {
    return m_running;
}

Benchmark::Stats Benchmark::stats() {
    return m_stats;
}
